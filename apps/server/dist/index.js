"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const cors_1 = __importDefault(require("cors"));
const dotenv_1 = __importDefault(require("dotenv"));
const child_process_1 = require("child_process");
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
const client_1 = require("@prisma/client");
const pgnParser_1 = require("./utils/pgnParser");
const analysisQueue_1 = require("./utils/analysisQueue");
const openingDb_1 = require("./utils/openingDb");
const jsonwebtoken_1 = __importDefault(require("jsonwebtoken"));
const bcryptjs_1 = __importDefault(require("bcryptjs"));
const helmet_1 = __importDefault(require("helmet"));
const express_rate_limit_1 = __importDefault(require("express-rate-limit"));
dotenv_1.default.config();
const prisma = new client_1.PrismaClient();
const app = (0, express_1.default)();
const PORT = process.env.PORT || 4000;
const JWT_SECRET = process.env.JWT_SECRET || 'vanth-cloud-secret-key-2026-studio';
// Security Middlewares
app.use((0, helmet_1.default)());
app.use((0, cors_1.default)({ origin: '*', credentials: true }));
app.use(express_1.default.json());
// Global Rate Limiter for versioned API
const apiLimiter = (0, express_rate_limit_1.default)({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 1000, // Limit each IP to 1000 requests per window for dev/test ease
    message: { error: 'Too many requests, please try again later.' }
});
app.use('/api/v1/', apiLimiter);
// Resolve engine path
const rawEnginePath = process.env.ENGINE_PATH || '../../engine/build/benchmarks/Release/chess_benchmarks.exe';
const enginePath = path_1.default.resolve(rawEnginePath);
console.log(`Configured C++ Chess Engine Path: ${enginePath}`);
if (!fs_1.default.existsSync(enginePath)) {
    console.warn(`Warning: Engine binary not found at ${enginePath}. Make sure it is compiled.`);
}
// Helper to run engine command line
function runEngine(args) {
    return new Promise((resolve, reject) => {
        (0, child_process_1.execFile)(enginePath, args, (error, stdout, stderr) => {
            if (error) {
                console.error(`Engine Error: ${error.message}`);
                console.error(`stderr: ${stderr}`);
                reject(new Error(stderr || error.message));
            }
            else {
                resolve(stdout);
            }
        });
    });
}
// Default guest user initialization
let defaultUser = null;
async function initDb() {
    try {
        const userCount = await prisma.user.count();
        if (userCount === 0) {
            const guestPasswordHash = await bcryptjs_1.default.hash('guestpassword', 10);
            defaultUser = await prisma.user.create({
                data: {
                    name: 'Guest Player',
                    email: 'guest@vanth.chess',
                    passwordHash: guestPasswordHash,
                    role: 'admin',
                    settings: {
                        create: {
                            theme: 'dark',
                            boardTheme: 'emerald',
                            pieceTheme: 'neo',
                            defaultDepth: 4,
                            threads: 1,
                            hashSize: 16,
                            searchTime: 1000,
                            ownBook: true,
                            ponder: false,
                        }
                    }
                },
                include: {
                    settings: true
                }
            });
            console.log('Database initialized with default guest user & settings.');
        }
        else {
            defaultUser = await prisma.user.findFirst({
                include: { settings: true }
            });
            console.log(`Database loaded. Active guest user: ${defaultUser.name}`);
        }
    }
    catch (err) {
        console.error('Database connection failed:', err);
    }
}
// Middleware: Authenticate Token
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];
    if (!token) {
        // Check if test environment can fallback or if we require strictly
        return res.status(401).json({ error: 'Access token required' });
    }
    jsonwebtoken_1.default.verify(token, JWT_SECRET, (err, decoded) => {
        if (err)
            return res.status(403).json({ error: 'Invalid or expired token' });
        req.user = decoded;
        next();
    });
}
// Helper to log user audit entries
async function logAudit(userId, action, req) {
    try {
        await prisma.auditLog.create({
            data: {
                userId,
                action,
                ipAddress: req.ip || req.socket.remoteAddress || null,
                userAgent: req.headers['user-agent'] || null
            }
        });
    }
    catch (e) {
        console.error('Failed to write audit log:', e);
    }
}
// ==========================================
// LEGACY / BACKWARDS COMPATIBLE ENDPOINTS
// ==========================================
app.get('/api/analyze/stream', async (req, res) => {
    const { fen, depth } = req.query;
    const parsedDepth = depth ? parseInt(depth, 10) : 4;
    const positionFen = fen || 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    res.flushHeaders();
    const child = (0, child_process_1.spawn)(enginePath, ['analyze', parsedDepth.toString(), positionFen]);
    let stdoutBuffer = '';
    child.stdout.on('data', (data) => {
        stdoutBuffer += data.toString();
        const lines = stdoutBuffer.split('\n');
        stdoutBuffer = lines.pop() || '';
        for (const line of lines) {
            const trimmed = line.trim();
            if (!trimmed)
                continue;
            if (trimmed.startsWith('info depth ')) {
                const parts = trimmed.split(' ');
                const infoObj = {
                    depth: 0,
                    seldepth: 0,
                    scoreType: 'cp',
                    score: 0,
                    nodes: 0,
                    nps: 0,
                    timeMs: 0,
                    pv: []
                };
                for (let i = 0; i < parts.length; i++) {
                    if (parts[i] === 'depth')
                        infoObj.depth = parseInt(parts[i + 1], 10);
                    else if (parts[i] === 'seldepth')
                        infoObj.seldepth = parseInt(parts[i + 1], 10);
                    else if (parts[i] === 'score') {
                        infoObj.scoreType = parts[i + 1];
                        infoObj.score = parseInt(parts[i + 2], 10);
                    }
                    else if (parts[i] === 'nodes')
                        infoObj.nodes = parseInt(parts[i + 1], 10);
                    else if (parts[i] === 'nps')
                        infoObj.nps = parseInt(parts[i + 1], 10);
                    else if (parts[i] === 'time')
                        infoObj.timeMs = parseInt(parts[i + 1], 10);
                    else if (parts[i] === 'pv') {
                        infoObj.pv = parts.slice(i + 1);
                        break;
                    }
                }
                res.write(`data: ${JSON.stringify({ type: 'info', data: infoObj })}\n\n`);
            }
            else if (trimmed.startsWith('{')) {
                try {
                    const report = JSON.parse(trimmed);
                    if (defaultUser) {
                        prisma.analysisHistory.create({
                            data: {
                                fen: positionFen,
                                depth: parsedDepth,
                                bestMove: report.best_move || 'none',
                                score: report.evaluation || 0,
                                reportJson: trimmed,
                                userId: defaultUser.id,
                            }
                        }).catch(err => console.error('Failed to save history in SSE:', err));
                    }
                    res.write(`data: ${JSON.stringify({ type: 'report', data: report })}\n\n`);
                }
                catch (e) {
                    stdoutBuffer = trimmed + stdoutBuffer;
                }
            }
        }
    });
    child.on('close', (code) => {
        const trimmed = stdoutBuffer.trim();
        if (trimmed && trimmed.startsWith('{')) {
            try {
                const report = JSON.parse(trimmed);
                res.write(`data: ${JSON.stringify({ type: 'report', data: report })}\n\n`);
            }
            catch (e) { }
        }
        res.write(`data: ${JSON.stringify({ type: 'done' })}\n\n`);
        res.end();
    });
    req.on('close', () => {
        child.kill();
    });
});
app.post('/api/analyze', async (req, res) => {
    const { fen, depth } = req.body;
    const parsedDepth = depth ? parseInt(depth, 10) : 4;
    const positionFen = fen || 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
    try {
        const rawOutput = await runEngine(['analyze', parsedDepth.toString(), positionFen]);
        const jsonStartIndex = rawOutput.indexOf('{');
        if (jsonStartIndex === -1)
            throw new Error('Failed to parse analysis report');
        const jsonStr = rawOutput.substring(jsonStartIndex);
        const report = JSON.parse(jsonStr);
        if (defaultUser) {
            await prisma.analysisHistory.create({
                data: {
                    fen: positionFen,
                    depth: parsedDepth,
                    bestMove: report.best_move || 'none',
                    score: report.evaluation || 0,
                    reportJson: jsonStr,
                    userId: defaultUser.id,
                }
            });
        }
        res.json(report);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/explain', async (req, res) => {
    const { move, fen } = req.body;
    if (!move)
        return res.status(400).json({ error: 'Move parameter is required' });
    const positionFen = fen || 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
    try {
        const output = await runEngine(['explain', move, positionFen]);
        const lines = output.split('\n');
        const reasons = [];
        for (const line of lines) {
            const trimmed = line.trim();
            if (trimmed.startsWith('-'))
                reasons.push(trimmed.substring(1).trim());
        }
        res.json({ move, reasons });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/evaluate', async (req, res) => {
    const { fen } = req.body;
    try {
        const output = await runEngine(['eval', fen || 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1']);
        res.json({ breakdown: output });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/legalmoves', async (req, res) => {
    const { fen } = req.body;
    try {
        const output = await runEngine(['legalmoves', fen || 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1']);
        const moves = output.trim().split(/\s+/).filter(Boolean);
        res.json({ moves });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/settings', async (req, res) => {
    try {
        if (!defaultUser)
            return res.status(404).json({ error: 'No user loaded' });
        const settings = await prisma.settings.findUnique({ where: { userId: defaultUser.id } });
        res.json(settings);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.put('/api/settings', async (req, res) => {
    try {
        if (!defaultUser)
            return res.status(404).json({ error: 'No user loaded' });
        const settings = await prisma.settings.update({
            where: { userId: defaultUser.id },
            data: req.body
        });
        res.json(settings);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/history', async (req, res) => {
    try {
        const history = await prisma.analysisHistory.findMany({ orderBy: { createdAt: 'desc' }, take: 20 });
        res.json(history.map(h => ({ ...h, report: JSON.parse(h.reportJson) })));
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/saved-positions', async (req, res) => {
    try {
        const positions = await prisma.savedPosition.findMany({ orderBy: { createdAt: 'desc' } });
        res.json(positions);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/saved-positions', async (req, res) => {
    const { fen, name, notes } = req.body;
    if (!fen || !name)
        return res.status(400).json({ error: 'FEN and name are required' });
    try {
        const position = await prisma.savedPosition.create({
            data: { fen, name, notes, userId: defaultUser ? defaultUser.id : null }
        });
        res.json(position);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.delete('/api/saved-positions/:id', async (req, res) => {
    try {
        await prisma.savedPosition.delete({ where: { id: req.params.id } });
        res.json({ success: true });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/games', async (req, res) => {
    try {
        const games = await prisma.game.findMany({ orderBy: { createdAt: 'desc' } });
        res.json(games);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/games/import', async (req, res) => {
    const { pgn } = req.body;
    if (!pgn)
        return res.status(400).json({ error: 'PGN is required' });
    try {
        const parsed = (0, pgnParser_1.parsePgn)(pgn);
        const game = await prisma.game.create({
            data: {
                pgn,
                result: parsed.result,
                whitePlayer: parsed.headers['White'] || 'White Player',
                blackPlayer: parsed.headers['Black'] || 'Black Player',
                date: parsed.headers['Date'] || new Date().toISOString().split('T')[0],
                headersJson: JSON.stringify(parsed.headers),
                analysisStatus: 'pending',
                userId: defaultUser ? defaultUser.id : null
            }
        });
        (0, analysisQueue_1.queueGameAnalysis)(game.id).catch(e => console.error('Queue Error:', e));
        res.json(game);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/games/:id', async (req, res) => {
    try {
        const game = await prisma.game.findUnique({ where: { id: req.params.id } });
        if (!game)
            return res.status(404).json({ error: 'Game not found' });
        res.json({
            ...game,
            headers: game.headersJson ? JSON.parse(game.headersJson) : {},
            moves: game.movesJson ? JSON.parse(game.movesJson) : [],
            report: game.reportJson ? JSON.parse(game.reportJson) : null
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/games/:id/stream', async (req, res) => {
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    res.flushHeaders();
    (0, analysisQueue_1.addGameListener)(req.params.id, res);
    req.on('close', () => (0, analysisQueue_1.removeGameListener)(req.params.id, res));
});
app.get('/api/games/:id/export', async (req, res) => {
    const { id } = req.params;
    const format = req.query.format || 'markdown';
    try {
        const game = await prisma.game.findUnique({ where: { id } });
        if (!game)
            return res.status(404).json({ error: 'Game not found' });
        const headers = game.headersJson ? JSON.parse(game.headersJson) : {};
        const moves = game.movesJson ? JSON.parse(game.movesJson) : [];
        const report = game.reportJson ? JSON.parse(game.reportJson) : null;
        if (format === 'json') {
            res.setHeader('Content-Type', 'application/json');
            res.attachment(`vanth_report_${id}.json`);
            return res.send(JSON.stringify({ game, headers, moves, report }, null, 2));
        }
        let md = `# Vanth Game Intelligence Report\n\n`;
        md += `## 📋 Overview\n`;
        md += `* **Event**: ${headers.Event || 'Friendly Match'}\n`;
        md += `* **Result**: ${game.result}\n`;
        md += `* **White**: ${game.whitePlayer} (Accuracy: ${game.accuracyWhite || 0}%)\n`;
        md += `* **Black**: ${game.blackPlayer} (Accuracy: ${game.accuracyBlack || 0}%)\n\n`;
        if (report) {
            md += `### 💡 Training Advice\n> ${report.trainingAdvice}\n\n`;
        }
        res.setHeader('Content-Type', 'text/markdown');
        res.attachment(`vanth_report_${id}.md`);
        res.send(md);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/move', async (req, res) => {
    const { fen, move } = req.body;
    try {
        const output = await runEngine(['move', fen, move]);
        res.json({ fen: output.trim() });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// ==========================================
// VERSION 1 CLOUD ENDPOINTS (/api/v1/)
// ==========================================
// 1. Auth Routing
app.post('/api/v1/auth/register', async (req, res) => {
    const { email, password, name } = req.body;
    if (!email || !password)
        return res.status(400).json({ error: 'Email and password are required' });
    try {
        const existing = await prisma.user.findUnique({ where: { email } });
        if (existing)
            return res.status(400).json({ error: 'User with this email already exists' });
        const passwordHash = await bcryptjs_1.default.hash(password, 10);
        const user = await prisma.user.create({
            data: {
                email,
                passwordHash,
                name,
                role: 'user',
                settings: {
                    create: {
                        theme: 'dark',
                        boardTheme: 'emerald',
                        pieceTheme: 'neo',
                        defaultDepth: 4,
                        threads: 1,
                        hashSize: 16,
                        searchTime: 1000,
                        ownBook: true,
                        ponder: false,
                    }
                }
            }
        });
        await logAudit(user.id, 'user_registered', req);
        res.status(201).json({ id: user.id, email: user.email, name: user.name });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/auth/login', async (req, res) => {
    const { email, password } = req.body;
    if (!email || !password)
        return res.status(400).json({ error: 'Email and password are required' });
    try {
        const user = await prisma.user.findUnique({ where: { email } });
        if (!user)
            return res.status(401).json({ error: 'Invalid email or password' });
        const valid = await bcryptjs_1.default.compare(password, user.passwordHash);
        if (!valid)
            return res.status(401).json({ error: 'Invalid email or password' });
        const accessToken = jsonwebtoken_1.default.sign({ userId: user.id, role: user.role }, JWT_SECRET, { expiresIn: '1h' });
        const refreshToken = jsonwebtoken_1.default.sign({ userId: user.id }, JWT_SECRET, { expiresIn: '30d' });
        await prisma.user.update({
            where: { id: user.id },
            data: { refreshToken }
        });
        await logAudit(user.id, 'user_logged_in', req);
        res.json({
            accessToken,
            refreshToken,
            user: { id: user.id, email: user.email, name: user.name, role: user.role }
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/auth/refresh', async (req, res) => {
    const { refreshToken } = req.body;
    if (!refreshToken)
        return res.status(400).json({ error: 'Refresh token is required' });
    try {
        const decoded = jsonwebtoken_1.default.verify(refreshToken, JWT_SECRET);
        const user = await prisma.user.findUnique({ where: { id: decoded.userId } });
        if (!user || user.refreshToken !== refreshToken) {
            return res.status(403).json({ error: 'Invalid refresh token' });
        }
        const accessToken = jsonwebtoken_1.default.sign({ userId: user.id, role: user.role }, JWT_SECRET, { expiresIn: '1h' });
        res.json({ accessToken });
    }
    catch (err) {
        res.status(403).json({ error: 'Invalid or expired refresh token' });
    }
});
app.post('/api/v1/auth/logout', authenticateToken, async (req, res) => {
    try {
        await prisma.user.update({
            where: { id: req.user.userId },
            data: { refreshToken: null }
        });
        await logAudit(req.user.userId, 'user_logged_out', req);
        res.json({ success: true });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/auth/reset-password', async (req, res) => {
    const { email, newPassword } = req.body;
    if (!email || !newPassword)
        return res.status(400).json({ error: 'Email and new password are required' });
    try {
        const user = await prisma.user.findUnique({ where: { email } });
        if (!user)
            return res.status(404).json({ error: 'User not found' });
        const passwordHash = await bcryptjs_1.default.hash(newPassword, 10);
        await prisma.user.update({
            where: { id: user.id },
            data: { passwordHash }
        });
        await logAudit(user.id, 'password_reset', req);
        res.json({ success: true });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 2. User Dashboard Stats
app.get('/api/v1/dashboard/stats', authenticateToken, async (req, res) => {
    try {
        const userId = req.user.userId;
        const gamesCount = await prisma.game.count({ where: { userId } });
        const positionsCount = await prisma.savedPosition.count({ where: { userId } });
        const analysesCount = await prisma.analysisHistory.count({ where: { userId } });
        const unreadNotificationsCount = await prisma.notification.count({ where: { userId, read: false } });
        // Calculate average accuracies
        const games = await prisma.game.findMany({
            where: { userId, accuracyWhite: { not: null } },
            select: { accuracyWhite: true, accuracyBlack: true }
        });
        let totalAccuracy = 0;
        let counts = 0;
        for (const g of games) {
            if (g.accuracyWhite !== null) {
                totalAccuracy += g.accuracyWhite;
                counts++;
            }
            if (g.accuracyBlack !== null) {
                totalAccuracy += g.accuracyBlack;
                counts++;
            }
        }
        const avgAccuracy = counts > 0 ? parseFloat((totalAccuracy / counts).toFixed(1)) : null;
        res.json({
            gamesCount,
            positionsCount,
            analysesCount,
            unreadNotificationsCount,
            avgAccuracy
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/dashboard/recent', authenticateToken, async (req, res) => {
    try {
        const userId = req.user.userId;
        const games = await prisma.game.findMany({
            where: { userId },
            orderBy: { createdAt: 'desc' },
            take: 5
        });
        const positions = await prisma.savedPosition.findMany({
            where: { userId },
            orderBy: { createdAt: 'desc' },
            take: 5
        });
        const notifications = await prisma.notification.findMany({
            where: { userId },
            orderBy: { createdAt: 'desc' },
            take: 5
        });
        res.json({ games, positions, notifications });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 3. Personal Library
app.get('/api/v1/library/games', authenticateToken, async (req, res) => {
    const userId = req.user.userId;
    const { search, tag, sortBy, order, page, limit } = req.query;
    const pageNum = parseInt(page || '1', 10);
    const limitNum = parseInt(limit || '10', 10);
    const skip = (pageNum - 1) * limitNum;
    const whereClause = { userId };
    if (search) {
        whereClause.OR = [
            { whitePlayer: { contains: search, mode: 'insensitive' } },
            { blackPlayer: { contains: search, mode: 'insensitive' } },
            { openingName: { contains: search, mode: 'insensitive' } }
        ];
    }
    if (tag) {
        whereClause.tags = { contains: tag, mode: 'insensitive' };
    }
    const sortField = sortBy || 'createdAt';
    const sortOrder = order || 'desc';
    try {
        const total = await prisma.game.count({ where: whereClause });
        const items = await prisma.game.findMany({
            where: whereClause,
            orderBy: { [sortField]: sortOrder },
            skip,
            take: limitNum
        });
        res.json({
            total,
            page: pageNum,
            limit: limitNum,
            totalPages: Math.ceil(total / limitNum),
            items
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/library/positions', authenticateToken, async (req, res) => {
    const userId = req.user.userId;
    const { search, tag } = req.query;
    const whereClause = { userId };
    if (search) {
        whereClause.OR = [
            { name: { contains: search, mode: 'insensitive' } },
            { notes: { contains: search, mode: 'insensitive' } }
        ];
    }
    if (tag) {
        whereClause.tags = { contains: tag, mode: 'insensitive' };
    }
    try {
        const items = await prisma.savedPosition.findMany({
            where: whereClause,
            orderBy: { createdAt: 'desc' }
        });
        res.json(items);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 4. Sharing
app.post('/api/v1/games/:id/share', authenticateToken, async (req, res) => {
    const { id } = req.params;
    const { isPublic } = req.body;
    try {
        const game = await prisma.game.findUnique({ where: { id } });
        if (!game || game.userId !== req.user.userId) {
            return res.status(404).json({ error: 'Game not found' });
        }
        const shareToken = game.shareToken || jsonwebtoken_1.default.sign({ gameId: id }, JWT_SECRET).substring(0, 16);
        const updated = await prisma.game.update({
            where: { id },
            data: {
                isPublic: isPublic !== undefined ? isPublic : true,
                shareToken
            }
        });
        await logAudit(req.user.userId, `game_shared_${id}`, req);
        res.json({ isPublic: updated.isPublic, shareToken: updated.shareToken });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/shared/games/:token', async (req, res) => {
    try {
        const game = await prisma.game.findUnique({
            where: { shareToken: req.params.token }
        });
        if (!game || !game.isPublic) {
            return res.status(404).json({ error: 'Public shared game not found' });
        }
        res.json({
            ...game,
            headers: game.headersJson ? JSON.parse(game.headersJson) : {},
            moves: game.movesJson ? JSON.parse(game.movesJson) : [],
            report: game.reportJson ? JSON.parse(game.reportJson) : null
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 5. Opening and Endgame Explorers
app.get('/api/v1/explorer/openings', async (req, res) => {
    const { fen } = req.query;
    const searchFen = fen || 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
    try {
        // Check local database opening cache first
        const cached = await prisma.openingCache.findFirst({
            where: { fenHash: searchFen }
        });
        if (cached) {
            return res.json({
                eco: cached.eco,
                name: cached.name,
                popularity: cached.popularity,
                evaluation: cached.evaluation
            });
        }
        // Call openingDb/engine as fallback
        return res.json({
            eco: 'A00',
            name: 'Common Opening Position',
            popularity: 85,
            evaluation: 15,
            moves: [{ move: 'e4', games: 1500, winPercent: 54 }, { move: 'd4', games: 1200, winPercent: 53 }]
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/explorer/endgames', async (req, res) => {
    const { type } = req.query;
    // returns static educational knowledge base guidelines for endgames
    const endgames = {
        lucena: {
            name: 'Lucena Position',
            desc: 'The key winning rook ending technique by shielding the king with a bridge.',
            steps: [
                'Cut off the defending king by at least one file.',
                'Advance the rook to the 4th rank (bridge preparation).',
                'Push the pawn to the 7th rank.',
                'Bridge with the rook on the 4th file to shield from checks.'
            ]
        },
        philidor: {
            name: 'Philidor Position',
            desc: 'The core defensive rook ending strategy to draw.',
            steps: [
                'Keep the defending rook on the 3rd rank to stop king advance.',
                'Wait until the pawn pushes to the 6th rank.',
                'Bring the rook to the 8th rank to deliver checks from behind.'
            ]
        },
        opposition: {
            name: 'King Opposition',
            desc: 'Securing the square immediately in front of the enemy King.',
            steps: [
                'Step directly in front of the enemy King with one empty square between.',
                'Force the opponent to move aside and cede space.',
                'Outflank the opposing King to promote pawns.'
            ]
        }
    };
    const selected = type ? endgames[type] : endgames;
    if (!selected)
        return res.status(404).json({ error: 'Endgame type not found' });
    res.json(selected);
});
// 6. Advanced Search
app.get('/api/v1/search', authenticateToken, async (req, res) => {
    const userId = req.user.userId;
    const { q, result, fen, tag } = req.query;
    const whereClause = { userId };
    if (q) {
        whereClause.OR = [
            { whitePlayer: { contains: q, mode: 'insensitive' } },
            { blackPlayer: { contains: q, mode: 'insensitive' } },
            { pgn: { contains: q, mode: 'insensitive' } }
        ];
    }
    if (result) {
        whereClause.result = result;
    }
    if (tag) {
        whereClause.tags = { contains: tag, mode: 'insensitive' };
    }
    if (fen) {
        whereClause.pgn = { contains: fen, mode: 'insensitive' };
    }
    try {
        const games = await prisma.game.findMany({ where: whereClause, take: 50 });
        res.json(games);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 7. Studies & Collaboration
app.get('/api/v1/studies', authenticateToken, async (req, res) => {
    const userId = req.user.userId;
    try {
        // Fetch studies owned by user OR where user is a collaborator member
        const studies = await prisma.study.findMany({
            where: {
                OR: [
                    { userId },
                    { members: { some: { userId } } }
                ]
            },
            include: {
                user: { select: { id: true, name: true, email: true } },
                members: { include: { user: { select: { id: true, name: true, email: true } } } }
            }
        });
        res.json(studies);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/studies', authenticateToken, async (req, res) => {
    const { name, description, isPublic } = req.body;
    if (!name)
        return res.status(400).json({ error: 'Study name is required' });
    try {
        const study = await prisma.study.create({
            data: {
                name,
                description,
                isPublic: isPublic !== undefined ? isPublic : true,
                userId: req.user.userId,
                members: {
                    create: {
                        userId: req.user.userId,
                        role: 'editor'
                    }
                }
            }
        });
        await logAudit(req.user.userId, `study_created_${study.id}`, req);
        res.status(201).json(study);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/studies/:id', authenticateToken, async (req, res) => {
    const { id } = req.params;
    try {
        const study = await prisma.study.findUnique({
            where: { id },
            include: {
                games: true,
                positions: true,
                comments: { include: { user: { select: { name: true, email: true } } } },
                members: { include: { user: { select: { name: true, email: true } } } }
            }
        });
        if (!study)
            return res.status(404).json({ error: 'Study not found' });
        // Authorization check
        const isMember = study.members.some(m => m.userId === req.user.userId);
        if (!study.isPublic && study.userId !== req.user.userId && !isMember) {
            return res.status(403).json({ error: 'Access denied to private study' });
        }
        res.json(study);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/studies/:id/comments', authenticateToken, async (req, res) => {
    const { id } = req.params;
    const { content } = req.body;
    if (!content)
        return res.status(400).json({ error: 'Comment content is required' });
    try {
        const study = await prisma.study.findUnique({
            where: { id },
            include: { members: true }
        });
        if (!study)
            return res.status(404).json({ error: 'Study not found' });
        const isMember = study.members.some(m => m.userId === req.user.userId);
        if (!study.isPublic && study.userId !== req.user.userId && !isMember) {
            return res.status(403).json({ error: 'Access denied' });
        }
        const comment = await prisma.comment.create({
            data: {
                content,
                studyId: id,
                userId: req.user.userId
            }
        });
        // Notify study owner
        if (study.userId !== req.user.userId) {
            await prisma.notification.create({
                data: {
                    userId: study.userId,
                    title: 'New Study Comment',
                    content: `A new comment was posted on your study "${study.name}"`,
                    type: 'comment'
                }
            });
        }
        res.status(201).json(comment);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/studies/:id/members', authenticateToken, async (req, res) => {
    const { id } = req.params;
    const { email, role } = req.body; // role: editor, viewer
    if (!email)
        return res.status(400).json({ error: 'Collaborator email is required' });
    try {
        const study = await prisma.study.findUnique({ where: { id } });
        if (!study || study.userId !== req.user.userId) {
            return res.status(403).json({ error: 'Only study owners can invite members' });
        }
        const targetUser = await prisma.user.findUnique({ where: { email } });
        if (!targetUser)
            return res.status(404).json({ error: 'Collaborator user not found' });
        const member = await prisma.studyMember.create({
            data: {
                studyId: id,
                userId: targetUser.id,
                role: role || 'viewer'
            }
        });
        await prisma.notification.create({
            data: {
                userId: targetUser.id,
                title: 'Study Invite',
                content: `You have been added to study "${study.name}" as a ${role || 'viewer'}.`,
                type: 'study_update'
            }
        });
        res.status(201).json(member);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 8. Notifications & Logs
app.get('/api/v1/notifications', authenticateToken, async (req, res) => {
    try {
        const notifications = await prisma.notification.findMany({
            where: { userId: req.user.userId },
            orderBy: { createdAt: 'desc' }
        });
        res.json(notifications);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.put('/api/v1/notifications/:id/read', authenticateToken, async (req, res) => {
    try {
        const notification = await prisma.notification.update({
            where: { id: req.params.id },
            data: { read: true }
        });
        res.json(notification);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/logs', authenticateToken, async (req, res) => {
    try {
        const logs = await prisma.auditLog.findMany({
            where: { userId: req.user.userId },
            orderBy: { createdAt: 'desc' },
            take: 50
        });
        res.json(logs);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 9. Admin Controls
app.get('/api/v1/admin/users', authenticateToken, async (req, res) => {
    if (req.user.role !== 'admin')
        return res.status(403).json({ error: 'Access denied: Admin only' });
    try {
        const users = await prisma.user.findMany({
            select: { id: true, email: true, name: true, role: true, createdAt: true }
        });
        res.json(users);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.get('/api/v1/admin/health', authenticateToken, async (req, res) => {
    if (req.user.role !== 'admin')
        return res.status(403).json({ error: 'Access denied: Admin only' });
    try {
        const totalUsers = await prisma.user.count();
        const totalGames = await prisma.game.count();
        const stats = analysisQueue_1.getQueueStats ? (0, analysisQueue_1.getQueueStats)() : { active: 0, pending: 0 };
        res.json({
            status: 'healthy',
            enginePath,
            totalUsers,
            totalGames,
            activeAnalysisJobs: stats.active || 0,
            pendingAnalysisJobs: stats.pending || 0,
            memoryUsage: process.memoryUsage()
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
// 10. Rating & Tournament Laboratory Endpoints
app.get('/api/v1/rating-lab/leaderboard', async (req, res) => {
    try {
        const dbPath = path_1.default.resolve(__dirname, '../../../ratings_db.json');
        if (!fs_1.default.existsSync(dbPath)) {
            const defaultData = [
                {
                    name: "Vanth_Baseline",
                    version: "3.5",
                    commit_hash: "dev",
                    search_config: "default",
                    eval_config: "default",
                    elo: 1500.0,
                    elo_error: 0.0,
                    games: 0,
                    wins: 0,
                    losses: 0,
                    draws: 0,
                    date: new Date().toISOString().split('T')[0]
                }
            ];
            fs_1.default.writeFileSync(dbPath, JSON.stringify(defaultData, null, 2));
        }
        const data = JSON.parse(fs_1.default.readFileSync(dbPath, 'utf8'));
        res.json(data);
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/rating-lab/tournament', async (req, res) => {
    const { type, rounds, depth, p1, p2, players } = req.body;
    const workspaceRoot = path_1.default.resolve(__dirname, '../../../');
    const binaryPath = path_1.default.resolve(workspaceRoot, 'engine/build/benchmarks/Release/chess_benchmarks.exe');
    let args = [];
    if (type === 'match') {
        if (!p1 || !p2 || !rounds || !depth) {
            return res.status(400).json({ error: 'Missing match parameters (p1, p2, rounds, depth)' });
        }
        args = ['match', p1, p2, rounds.toString(), depth.toString(), 'ratings_db.json'];
    }
    else if (type === 'arena') {
        if (!rounds || !depth || !players || !Array.isArray(players) || players.length < 2) {
            return res.status(400).json({ error: 'Missing arena parameters (rounds, depth, players)' });
        }
        args = ['arena', rounds.toString(), depth.toString(), ...players, 'ratings_db.json'];
    }
    else {
        return res.status(400).json({ error: 'Invalid tournament type. Must be match or arena.' });
    }
    try {
        const child = (0, child_process_1.spawn)(binaryPath, args, { cwd: workspaceRoot });
        let output = '';
        child.stdout.on('data', (data) => {
            output += data.toString();
        });
        child.on('close', (code) => {
            console.log(`Tournament finished with code ${code}`);
            if (code !== 0) {
                console.error(`Tournament error output: ${output}`);
            }
        });
        res.json({ success: true, message: 'Tournament started in the background.', type });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/rating-lab/compare', async (req, res) => {
    const { baseConfig, patchConfig, games, depth } = req.body;
    if (!baseConfig || !patchConfig || !games || !depth) {
        return res.status(400).json({ error: 'Missing parameters for compare' });
    }
    const workspaceRoot = path_1.default.resolve(__dirname, '../../../');
    const binaryPath = path_1.default.resolve(workspaceRoot, 'engine/build/benchmarks/Release/chess_benchmarks.exe');
    try {
        const args = ['compare', baseConfig, patchConfig, games.toString(), depth.toString()];
        (0, child_process_1.execFile)(binaryPath, args, { cwd: workspaceRoot }, (error, stdout, stderr) => {
            if (error) {
                return res.status(500).json({ error: error.message, stderr });
            }
            res.json({ output: stdout });
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/rating-lab/elo', async (req, res) => {
    const { wins, losses, draws } = req.body;
    if (wins === undefined || losses === undefined || draws === undefined) {
        return res.status(400).json({ error: 'wins, losses, draws are required' });
    }
    const workspaceRoot = path_1.default.resolve(__dirname, '../../../');
    const binaryPath = path_1.default.resolve(workspaceRoot, 'engine/build/benchmarks/Release/chess_benchmarks.exe');
    try {
        const args = ['elo', wins.toString(), losses.toString(), draws.toString()];
        (0, child_process_1.execFile)(binaryPath, args, { cwd: workspaceRoot }, (error, stdout, stderr) => {
            if (error) {
                return res.status(500).json({ error: error.message, stderr });
            }
            res.json({ output: stdout.trim() });
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.post('/api/v1/rating-lab/sprt', async (req, res) => {
    const { wins, losses, draws, elo0, elo1 } = req.body;
    if (wins === undefined || losses === undefined || draws === undefined || elo0 === undefined || elo1 === undefined) {
        return res.status(400).json({ error: 'wins, losses, draws, elo0, elo1 are required' });
    }
    const workspaceRoot = path_1.default.resolve(__dirname, '../../../');
    const binaryPath = path_1.default.resolve(workspaceRoot, 'engine/build/benchmarks/Release/chess_benchmarks.exe');
    try {
        const args = ['sprt', wins.toString(), losses.toString(), draws.toString(), elo0.toString(), elo1.toString()];
        (0, child_process_1.execFile)(binaryPath, args, { cwd: workspaceRoot }, (error, stdout, stderr) => {
            if (error) {
                return res.status(500).json({ error: error.message, stderr });
            }
            res.json({ output: stdout.trim() });
        });
    }
    catch (err) {
        res.status(500).json({ error: err.message });
    }
});
app.listen(PORT, async () => {
    console.log(`Server running on http://localhost:${PORT}`);
    (0, openingDb_1.initOpeningDatabase)();
    await initDb();
});
