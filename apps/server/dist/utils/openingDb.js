"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.initOpeningDatabase = initOpeningDatabase;
exports.detectOpening = detectOpening;
const chess_js_1 = require("chess.js");
const openingsList = [
    { name: "Ruy Lopez", eco: "C60", moves: ["e4", "e5", "Nf3", "Nc6", "Bb5"] },
    { name: "Ruy Lopez, Berlin Defence", eco: "C65", moves: ["e4", "e5", "Nf3", "Nc6", "Bb5", "Nf6"] },
    { name: "Sicilian Defence", eco: "B20", moves: ["e4", "c5"] },
    { name: "Sicilian Defence, Najdorf", eco: "B90", moves: ["e4", "c5", "Nf3", "d6", "d4", "cxd4", "Nxd4", "Nf6", "Nc3", "a6"] },
    { name: "Sicilian Defence, Dragon", eco: "B70", moves: ["e4", "c5", "Nf3", "d6", "d4", "cxd4", "Nxd4", "Nf6", "Nc3", "g6"] },
    { name: "French Defence", eco: "C00", moves: ["e4", "e6"] },
    { name: "French Defence, Advance Variation", eco: "C02", moves: ["e4", "e6", "d4", "d5", "e5"] },
    { name: "Caro-Kann Defence", eco: "B10", moves: ["e4", "c6"] },
    { name: "Caro-Kann Defence, Classical", eco: "B18", moves: ["e4", "c6", "d4", "d5", "Nc3", "dxe4", "Nxe4", "Bf5"] },
    { name: "Queen's Gambit", eco: "D06", moves: ["d4", "d5", "c4"] },
    { name: "Queen's Gambit Declined", eco: "D30", moves: ["d4", "d5", "c4", "e6"] },
    { name: "Queen's Gambit Accepted", eco: "D20", moves: ["d4", "d5", "c4", "dxc4"] },
    { name: "King's Indian Defence", eco: "E60", moves: ["d4", "Nf6", "c4", "g6"] },
    { name: "Slav Defence", eco: "D10", moves: ["d4", "d5", "c4", "c6"] },
    { name: "Scandinavian Defence", eco: "B01", moves: ["e4", "d5"] },
    { name: "Italian Game", eco: "C50", moves: ["e4", "e5", "Nf3", "Nc6", "Bc4"] },
    { name: "Italian Game, Evans Gambit", eco: "C51", moves: ["e4", "e5", "Nf3", "Nc6", "Bc4", "Bc5", "b4"] },
    { name: "English Opening", eco: "A10", moves: ["c4"] },
    { name: "Reti Opening", eco: "A04", moves: ["Nf3"] },
    { name: "Scotch Game", eco: "C44", moves: ["e4", "e5", "Nf3", "Nc6", "d4"] },
    { name: "Nimzo-Indian Defence", eco: "E20", moves: ["d4", "Nf6", "c4", "e6", "Nc3", "Bb4"] },
    { name: "Grünfeld Defence", eco: "D80", moves: ["d4", "Nf6", "c4", "g6", "Nc3", "d5"] },
    { name: "Alekhine's Defence", eco: "B02", moves: ["e4", "Nf6"] },
    { name: "Pirc Defence", eco: "B07", moves: ["e4", "d6", "d4", "Nf6", "Nc3", "g6"] },
    { name: "Vienna Game", eco: "C30", moves: ["e4", "e5", "Nc3"] }
];
const openingCache = new Map();
// Generate the cache key by stripping halfmove & fullmove counters
function getPositionKey(fen) {
    return fen.split(' ').slice(0, 4).join(' ');
}
function initOpeningDatabase() {
    for (const op of openingsList) {
        const chess = new chess_js_1.Chess();
        let valid = true;
        for (const m of op.moves) {
            try {
                chess.move(m);
            }
            catch (e) {
                valid = false;
                break;
            }
        }
        if (valid) {
            const key = getPositionKey(chess.fen());
            openingCache.set(key, { name: op.name, eco: op.eco });
        }
    }
}
function detectOpening(fen) {
    if (openingCache.size === 0) {
        initOpeningDatabase();
    }
    const key = getPositionKey(fen);
    return openingCache.get(key) || null;
}
