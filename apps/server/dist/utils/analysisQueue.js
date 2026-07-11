"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.getQueueStats = getQueueStats;
exports.addGameListener = addGameListener;
exports.removeGameListener = removeGameListener;
exports.queueGameAnalysis = queueGameAnalysis;
const client_1 = require("@prisma/client");
const chess_js_1 = require("chess.js");
const child_process_1 = require("child_process");
const path_1 = __importDefault(require("path"));
const pgnParser_1 = require("./pgnParser");
const openingDb_1 = require("./openingDb");
const endgameDb_1 = require("./endgameDb");
const moveClassifier_1 = require("./moveClassifier");
const prisma = new client_1.PrismaClient();
// Setup engine path
const isDev = process.env.NODE_ENV !== 'production';
const engineFilename = process.platform === 'win32' ? 'chess_benchmarks.exe' : 'chess_benchmarks';
const enginePath = path_1.default.resolve(__dirname, '../../../../engine/build/benchmarks', isDev ? 'Release' : 'Release', engineFilename);
// Map to track SSE connections listening to specific games
const sseListeners = new Map();
let activeJobsCount = 0;
let pendingJobsCount = 0;
function getQueueStats() {
    return {
        active: activeJobsCount,
        pending: pendingJobsCount
    };
}
function addGameListener(gameId, res) {
    if (!sseListeners.has(gameId)) {
        sseListeners.set(gameId, []);
    }
    sseListeners.get(gameId).push(res);
}
function removeGameListener(gameId, res) {
    const listeners = sseListeners.get(gameId);
    if (listeners) {
        sseListeners.set(gameId, listeners.filter(l => l !== res));
    }
}
function broadcastProgress(gameId, data) {
    const listeners = sseListeners.get(gameId);
    if (listeners) {
        for (const res of listeners) {
            res.write(`data: ${JSON.stringify(data)}\n\n`);
        }
    }
}
// Spawns engine process synchronously to retrieve report
function analyzePosition(fen, depth) {
    return new Promise((resolve, reject) => {
        const child = (0, child_process_1.spawn)(enginePath, ['analyze', depth.toString(), fen]);
        let output = '';
        child.stdout.on('data', (data) => {
            output += data.toString();
        });
        child.on('close', (code) => {
            if (code !== 0) {
                reject(new Error(`Engine analyze process failed with code ${code}`));
                return;
            }
            try {
                const jsonStart = output.indexOf('{');
                if (jsonStart === -1) {
                    reject(new Error('No JSON output from engine'));
                    return;
                }
                const report = JSON.parse(output.substring(jsonStart));
                resolve(report);
            }
            catch (e) {
                reject(e);
            }
        });
    });
}
// Background analysis worker loop
async function queueGameAnalysis(gameId, searchDepth = 4) {
    pendingJobsCount++;
    let wasActive = false;
    try {
        const game = await prisma.game.findUnique({ where: { id: gameId } });
        if (!game)
            return;
        pendingJobsCount = Math.max(0, pendingJobsCount - 1);
        activeJobsCount++;
        wasActive = true;
        console.log(`[Queue] Starting background game analysis for Game ID: ${gameId}`);
        // Update game status to processing
        await prisma.game.update({
            where: { id: gameId },
            data: { analysisStatus: 'processing' }
        });
        // Replay game using chess.js
        const parsedPgn = (0, pgnParser_1.parsePgn)(game.pgn);
        const chess = new chess_js_1.Chess();
        // Flatten moves to sequential ply list for analytical indexing
        const plys = [];
        // Flat walker (ignores branching variations for core timeline analysis)
        const traverseMoves = (moveList) => {
            for (const m of moveList) {
                const fenBefore = chess.fen();
                try {
                    const result = chess.move(m.san);
                    plys.push({
                        san: m.san,
                        uci: result.from + result.to + (result.promotion || ''),
                        comments: m.comments,
                        nags: m.nags,
                        fenBefore,
                        fenAfter: chess.fen()
                    });
                }
                catch (e) {
                    // If malformed, skip to avoid crashing queue
                    break;
                }
            }
        };
        traverseMoves(parsedPgn.moves);
        const totalPlys = plys.length;
        const movesJsonArray = [];
        let prevEvaluation = 15; // Starting position score (~0.15 pawns for White)
        let totalAccuracyWhite = 0;
        let totalAccuracyBlack = 0;
        let whiteMoveCount = 0;
        let blackMoveCount = 0;
        let whiteMistakes = 0;
        let blackMistakes = 0;
        let whiteBlunders = 0;
        let blackBlunders = 0;
        let whiteBrilliants = 0;
        let blackBrilliants = 0;
        for (let idx = 0; idx < totalPlys; idx++) {
            const ply = plys[idx];
            const isWhiteMove = idx % 2 === 0;
            // 1. Check database cache to avoid repeating C++ spawns
            let report = null;
            const cached = await prisma.analysisHistory.findFirst({
                where: { fen: ply.fenAfter }
            });
            if (cached) {
                try {
                    report = JSON.parse(cached.reportJson);
                }
                catch (e) { }
            }
            // 2. Fallback to active engine analyze
            if (!report) {
                try {
                    report = await analyzePosition(ply.fenAfter, searchDepth);
                    // Save to database cache
                    await prisma.analysisHistory.create({
                        data: {
                            fen: ply.fenAfter,
                            depth: searchDepth,
                            bestMove: report.best_move || 'none',
                            score: report.evaluation || 0,
                            reportJson: JSON.stringify(report)
                        }
                    });
                }
                catch (e) {
                    console.error(`[Queue] Failed to analyze ply ${idx}:`, e);
                    // Fallback empty report
                    report = {
                        fen: ply.fenAfter,
                        best_move: '',
                        evaluation: prevEvaluation,
                        breakdown: { material: 0, pst: 0, pawn_structure: 0, mobility: 0, piece_activity: 0, king_safety: 0, space: 0, threats: 0, endgame: 0, tempo: 0, total: prevEvaluation },
                        alternatives: [],
                        explanation: { move: ply.san, reasons: [] },
                        tactical_themes: { fork: false, pin: false, skewer: false, discovered_attack: false, double_attack: false, deflection: false, decoy: false, x_ray: false, back_rank_weakness: false, remove_defender: false, zwischenzug: false, descriptions: [] },
                        classifications: { opening: false, middlegame: false, endgame: false, closed_position: false, open_position: false, semi_open: false, tactical: false, strategic: false, king_attack: false, pawn_race: false, labels: [] },
                        strengths: [],
                        weaknesses: [],
                        search_stats: { nodes: 0, nps: 0, time_ms: 0, cutoffs: 0, tt_hits: 0 }
                    };
                }
            }
            // Get best move report for diffing
            let bestReport = report;
            if (report.best_move && report.best_move !== ply.uci) {
                // Find or run best move report
                try {
                    const tempChess = new chess_js_1.Chess(ply.fenBefore);
                    tempChess.move(report.best_move);
                    const bestFen = tempChess.fen();
                    let cachedBest = await prisma.analysisHistory.findFirst({ where: { fen: bestFen } });
                    if (cachedBest) {
                        bestReport = JSON.parse(cachedBest.reportJson);
                    }
                    else {
                        bestReport = await analyzePosition(bestFen, searchDepth);
                    }
                }
                catch (e) { }
            }
            // 3. Move Classification & Mistake Detection
            const isBookMove = !!(0, openingDb_1.detectOpening)(ply.fenAfter);
            const tempGame = new chess_js_1.Chess(ply.fenBefore);
            const legalCount = tempGame.moves().length;
            const classification = (0, moveClassifier_1.classifyMove)(ply.san, report.best_move || ply.san, report, bestReport, prevEvaluation, isBookMove, legalCount);
            // Accumulate stats
            if (isWhiteMove) {
                whiteMoveCount++;
                totalAccuracyWhite += Math.max(0, 100 - classification.delta);
                if (classification.classification === 'Mistake')
                    whiteMistakes++;
                else if (classification.classification === 'Blunder')
                    whiteBlunders++;
                else if (classification.classification === 'Brilliant')
                    whiteBrilliants++;
            }
            else {
                blackMoveCount++;
                totalAccuracyBlack += Math.max(0, 100 - classification.delta);
                if (classification.classification === 'Mistake')
                    blackMistakes++;
                else if (classification.classification === 'Blunder')
                    blackBlunders++;
                else if (classification.classification === 'Brilliant')
                    blackBrilliants++;
            }
            const moveRecord = {
                moveIndex: idx + 1,
                san: ply.san,
                uci: ply.uci,
                fen: ply.fenAfter,
                score: report.evaluation,
                delta: classification.delta,
                classification: classification.classification,
                bestMove: report.best_move,
                tacticalMistake: classification.tacticalMistake,
                positionalMistake: classification.positionalMistake,
                explanation: classification.explanation,
                breakdown: report.breakdown,
                tacticalThemes: report.tactical_themes.descriptions,
                comments: ply.comments,
                nags: ply.nags
            };
            movesJsonArray.push(moveRecord);
            prevEvaluation = report.evaluation;
            // Broadcast progress
            const progressPercent = Math.round(((idx + 1) / totalPlys) * 100);
            broadcastProgress(gameId, {
                type: 'progress',
                progress: progressPercent,
                currentMove: idx + 1,
                totalMoves: totalPlys,
                latestMove: moveRecord
            });
        }
        // 4. Generate Final Professional Game Report
        const accuracyWhite = whiteMoveCount > 0 ? Math.round(totalAccuracyWhite / whiteMoveCount) : 100;
        const accuracyBlack = blackMoveCount > 0 ? Math.round(totalAccuracyBlack / blackMoveCount) : 100;
        // Detect openings
        const firstFewFen = plys.length > 4 ? plys[3].fenAfter : (plys.length > 0 ? plys[plys.length - 1].fenAfter : '');
        const openingInfo = firstFewFen ? (0, openingDb_1.detectOpening)(firstFewFen) : null;
        // Detect endgames
        const lastFen = plys.length > 0 ? plys[plys.length - 1].fenAfter : '';
        const endgameInfo = lastFen ? (0, endgameDb_1.detectEndgame)(lastFen) : null;
        // Build training advice
        let trainingAdvice = "Your middle game play was structurally solid. Focus on maintaining piece coordination under pressure.";
        if (whiteBlunders + whiteMistakes > 3) {
            trainingAdvice = "Watch out for sudden tactical forks and double attacks. Double check your safety coordinates before piece trades.";
        }
        else if (whiteMoveCount > 10 && totalAccuracyWhite / whiteMoveCount < 70) {
            trainingAdvice = "Focus on developing your minor pieces to active squares early and controlling center lanes.";
        }
        const reportJsonObj = {
            accuracyWhite,
            accuracyBlack,
            whiteMistakes,
            blackMistakes,
            whiteBlunders,
            blackBlunders,
            whiteBrilliants,
            blackBrilliants,
            opening: openingInfo ? `${openingInfo.name} (${openingInfo.eco})` : 'Unknown Opening',
            endgame: endgameInfo ? `${endgameInfo.type}: ${endgameInfo.advice}` : 'No theoretical endgame reached.',
            trainingAdvice,
            strengths: ["Strong development in the opening phase", "Solid spatial control in the center"],
            weaknesses: whiteBlunders > 0 ? ["Tactical oversights under pressure"] : ["Quiet positional inaccuracies"]
        };
        // Update database record
        await prisma.game.update({
            where: { id: gameId },
            data: {
                analysisStatus: 'completed',
                movesJson: JSON.stringify(movesJsonArray),
                reportJson: JSON.stringify(reportJsonObj),
                accuracyWhite,
                accuracyBlack,
                eco: openingInfo?.eco || null,
                openingName: openingInfo?.name || null
            }
        });
        // Notify listeners
        broadcastProgress(gameId, {
            type: 'completed',
            report: reportJsonObj,
            moves: movesJsonArray
        });
        console.log(`[Queue] Completed analysis for Game ID: ${gameId}`);
    }
    catch (err) {
        console.error(`[Queue] Error during game analysis execution for Game ID ${gameId}:`, err);
        await prisma.game.update({
            where: { id: gameId },
            data: { analysisStatus: 'failed' }
        });
        broadcastProgress(gameId, { type: 'failed', error: String(err) });
    }
    finally {
        if (wasActive) {
            activeJobsCount = Math.max(0, activeJobsCount - 1);
        }
        else {
            pendingJobsCount = Math.max(0, pendingJobsCount - 1);
        }
    }
}
