"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.detectEndgame = detectEndgame;
function detectEndgame(fen) {
    const [boardPart, activeColor] = fen.split(' ');
    // Count pieces
    let wPieces = { p: 0, n: 0, b: 0, r: 0, q: 0, k: 0 };
    let bPieces = { p: 0, n: 0, b: 0, r: 0, q: 0, k: 0 };
    for (const char of boardPart) {
        if (char === 'P')
            wPieces.p++;
        else if (char === 'N')
            wPieces.n++;
        else if (char === 'B')
            wPieces.b++;
        else if (char === 'R')
            wPieces.r++;
        else if (char === 'Q')
            wPieces.q++;
        else if (char === 'K')
            wPieces.k++;
        else if (char === 'p')
            bPieces.p++;
        else if (char === 'n')
            bPieces.n++;
        else if (char === 'b')
            bPieces.b++;
        else if (char === 'r')
            bPieces.r++;
        else if (char === 'q')
            bPieces.q++;
        else if (char === 'k')
            bPieces.k++;
    }
    const wTotal = wPieces.p + wPieces.n + wPieces.b + wPieces.r + wPieces.q;
    const bTotal = bPieces.p + bPieces.n + bPieces.b + bPieces.r + bPieces.q;
    // 1. King + Pawn vs King
    if ((wPieces.p === 1 && wTotal === 1 && bTotal === 0) || (bPieces.p === 1 && bTotal === 1 && wTotal === 0)) {
        const sideWithPawn = wPieces.p === 1 ? 'White' : 'Black';
        return {
            type: "King & Pawn vs King",
            description: `A fundamental endgame where ${sideWithPawn} attempts to promote their single remaining pawn.`,
            advice: "The defender must keep their king in front of the pawn. The attacker must occupy key squares in front of the pawn (Opposition) to guide it to promotion."
        };
    }
    // 2. Lucena / Philidor Candidate (King + Rook + Pawn vs King + Rook)
    if (wPieces.r === 1 && wPieces.p === 1 && wTotal === 2 && bPieces.r === 1 && bTotal === 1) {
        return {
            type: "Theoretical Rook & Pawn Ending",
            description: "A Rook and Pawn endgame containing characteristics of the Lucena (winning) or Philidor (drawing) theoretical setups.",
            advice: "Attacker: Try to cut off the enemy king by 3+ files and use the 'building a bridge' technique (Lucena). Defender: Keep the defending king on the promotion file, and use the 3rd rank cut-off to prevent the king's advance (Philidor)."
        };
    }
    // 3. Basic Checkmates: King + Queen vs King
    if ((wPieces.q === 1 && wTotal === 1 && bTotal === 0) || (bPieces.q === 1 && bTotal === 1 && wTotal === 0)) {
        const attacker = wPieces.q === 1 ? 'White' : 'Black';
        return {
            type: "King & Queen vs King Mate",
            description: "A forced checkmate endgame where the lone king is cornered by the queen and king.",
            advice: `Corner the defending king to the edge of the board using a 'box' technique, then bring the ${attacker} king closer to deliver a supported checkmate (e.g. on the back rank or file). Beware of stalemate!`
        };
    }
    // 4. Basic Checkmates: King + Rook vs King
    if ((wPieces.r === 1 && wTotal === 1 && bTotal === 0) || (bPieces.r === 1 && bTotal === 1 && wTotal === 0)) {
        const attacker = wPieces.r === 1 ? 'White' : 'Black';
        return {
            type: "King & Rook vs King Mate",
            description: "A basic mating endgame requiring coordination between the rook and attacking king.",
            advice: `Cut off the defending king using the rook. Bring the ${attacker} king directly in front of the defending king (Opposition) and deliver a back-rank rook check to push the defending king back. Repeat until mate.`
        };
    }
    // 5. Bishop & Knight Mate (KBNK)
    if ((wPieces.b === 1 && wPieces.n === 1 && wTotal === 2 && bTotal === 0) ||
        (bPieces.b === 1 && bPieces.n === 1 && bTotal === 2 && wTotal === 0)) {
        return {
            type: "King, Bishop & Knight vs King",
            description: "One of the most complex basic checkmates. Mate can only be forced in a corner of the same color as the bishop.",
            advice: "Drive the defending king to the edge, then herd it to the correct corner (matching the bishop's color) using the 'W-maneuver' with the knight."
        };
    }
    // 6. Two Bishops Mate (KBBK)
    if ((wPieces.b === 2 && wTotal === 2 && bTotal === 0) || (bPieces.b === 2 && bTotal === 2 && wTotal === 0)) {
        return {
            type: "King & Two Bishops vs King",
            description: "A straightforward forced checkmate where two bishops cut off diagonal escape routes.",
            advice: "Use the two bishops side-by-side to create an impassable diagonal barrier. Push the defending king to the corner, bring the attacking king to support, and checkmate."
        };
    }
    return null;
}
