"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.classifyMove = classifyMove;
function classifyMove(playedMove, bestMove, playedReport, bestReport, prevEvaluation, isBook, legalMovesCount) {
    // Calculate evaluation delta
    // White tries to maximize score, Black tries to minimize.
    // To make it side-independent, we read relative centipawn drops.
    const activeColor = playedReport.fen.split(' ')[1];
    const isWhite = activeColor === 'b'; // playedReport.fen is the state AFTER the move, so active color has flipped!
    const scoreBefore = isWhite ? prevEvaluation : -prevEvaluation;
    const scorePlayed = isWhite ? playedReport.evaluation : -playedReport.evaluation;
    const scoreBest = isWhite ? bestReport.evaluation : -bestReport.evaluation;
    // Delta is the difference between playing the best move vs the played move
    const delta = scoreBest - scorePlayed;
    let classification = "Good";
    let explanation = "A solid, standard move maintaining the positional status quo.";
    let tacticalMistake;
    let positionalMistake;
    // 1. Book Move check
    if (isBook) {
        return {
            classification: "Book",
            delta: 0,
            explanation: "Following established chess opening theory."
        };
    }
    // 2. Forced Move check
    if (legalMovesCount === 1) {
        return {
            classification: "Forced",
            delta: 0,
            explanation: "The only legal move available to escape check or blocking threats."
        };
    }
    // 3. Best Move check
    if (playedMove.toLowerCase() === bestMove.toLowerCase()) {
        return {
            classification: "Best",
            delta: 0,
            explanation: "The absolute best move in the position, matching the engine's top choice."
        };
    }
    // 4. Brilliant Sacrifice check
    // Material drops significantly, but total evaluation increases or remains highly positive
    const matBefore = playedReport.breakdown.material;
    const matAfter = bestReport.breakdown.material;
    const isMaterialSacrifice = isWhite ? (matAfter - matBefore > 150) : (matBefore - matAfter > 150);
    if (isMaterialSacrifice && scorePlayed > 150 && delta < 15) {
        return {
            classification: "Brilliant",
            delta,
            explanation: "A brilliant sacrifice that gives up material to secure a massive tactical or positional advantage!"
        };
    }
    // 5. Missed Win / Mate check
    const wasWinning = isWhite ? prevEvaluation > 250 : prevEvaluation < -250;
    const isNowEqualOrLost = isWhite ? playedReport.evaluation < 80 : playedReport.evaluation > -80;
    if (wasWinning && isNowEqualOrLost && delta > 150) {
        classification = "Missed Win";
        explanation = "Missed a clear winning advantage, allowing the opponent to coordinate counterplay and level the game.";
    }
    // 6. Blunders, Mistakes, Inaccuracies
    else if (delta > 200) {
        classification = "Blunder";
        explanation = "A critical error that severely damages your position, leading to material loss or checkmate threats.";
    }
    else if (delta > 90) {
        classification = "Mistake";
        explanation = "A notable mistake that concedes the initiative or weakens your defensive structure.";
    }
    else if (delta > 30) {
        classification = "Inaccuracy";
        explanation = "An inaccuracy that slips away from optimal play, giving your opponent slight positional breathing room.";
    }
    else if (delta <= 10) {
        classification = "Excellent";
        explanation = "An excellent move that is nearly as strong as the engine's primary line.";
    }
    else {
        classification = "Good";
        explanation = "A respectable move that keeps the position balanced.";
    }
    // --- Tactical & Positional Mistake Detection ---
    if (delta > 30) {
        // A. Tactical Mistake checks
        const tacticalDiffs = playedReport.tactical_themes;
        const bestTactical = bestReport.tactical_themes;
        if (bestTactical.fork && !tacticalDiffs.fork) {
            tacticalMistake = "Fork opportunity missed";
        }
        else if (bestTactical.pin && !tacticalDiffs.pin) {
            tacticalMistake = "Pin opportunity ignored";
        }
        else if (bestTactical.skewer && !tacticalDiffs.skewer) {
            tacticalMistake = "Skewer opportunity missed";
        }
        else if (tacticalDiffs.back_rank_weakness && !bestTactical.back_rank_weakness) {
            tacticalMistake = "Created back-rank vulnerability";
        }
        // B. Positional Mistake checks (comparing subscore diffs)
        const breakdownPlayed = playedReport.breakdown;
        const breakdownBest = bestReport.breakdown;
        const kingSafetyLoss = isWhite
            ? (breakdownBest.king_safety - breakdownPlayed.king_safety)
            : (breakdownPlayed.king_safety - breakdownBest.king_safety);
        const spaceLoss = isWhite
            ? (breakdownBest.space - breakdownPlayed.space)
            : (breakdownPlayed.space - breakdownBest.space);
        const pawnStructLoss = isWhite
            ? (breakdownBest.pawn_structure - breakdownPlayed.pawn_structure)
            : (breakdownPlayed.pawn_structure - breakdownBest.pawn_structure);
        const mobilityLoss = isWhite
            ? (breakdownBest.mobility - breakdownPlayed.mobility)
            : (breakdownPlayed.mobility - breakdownBest.mobility);
        if (kingSafetyLoss > 40) {
            positionalMistake = "Weakened king shelter";
        }
        else if (pawnStructLoss > 30) {
            positionalMistake = "Weakened pawn structure";
        }
        else if (spaceLoss > 30) {
            positionalMistake = "Ceded valuable board space";
        }
        else if (mobilityLoss > 30) {
            positionalMistake = "Restricted piece mobility (poor development)";
        }
    }
    return {
        classification,
        delta,
        tacticalMistake,
        positionalMistake,
        explanation
    };
}
