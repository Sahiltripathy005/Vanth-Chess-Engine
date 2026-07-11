"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parsePgn = parsePgn;
function parsePgn(pgnString) {
    const headers = {};
    // 1. Extract Headers
    const headerRegex = /\[(\w+)\s+"([^"]*)"\]/g;
    let headerMatch;
    while ((headerMatch = headerRegex.exec(pgnString)) !== null) {
        headers[headerMatch[1]] = headerMatch[2];
    }
    // Remove headers specifically at start of lines to preserve bracketed clock info inside comments
    let movesPart = pgnString.replace(/^\[\w+\s+"[^"]*"\]/gm, '').trim();
    // 2. Tokenize and Parse Moves State Machine
    const moves = [];
    // Stack for handling nested variations
    const stack = [moves];
    let currentList = moves;
    let i = 0;
    const len = movesPart.length;
    while (i < len) {
        const char = movesPart[i];
        // Skip whitespace
        if (/\s/.test(char)) {
            i++;
            continue;
        }
        // Skip game result *
        if (char === '*') {
            i++;
            continue;
        }
        // Skip move numbers (e.g., "1.", "1...", "12.")
        if (/\d/.test(char)) {
            const remaining = movesPart.slice(i);
            if (remaining.startsWith("1-0") || remaining.startsWith("0-1") || remaining.startsWith("1/2-1/2")) {
                i += remaining.startsWith("1/2-1/2") ? 7 : 3;
                continue;
            }
            let numStr = '';
            while (i < len && (/\d/.test(movesPart[i]) || movesPart[i] === '.')) {
                numStr += movesPart[i];
                i++;
            }
            continue;
        }
        // Comments {...}
        if (char === '{') {
            i++;
            let comment = '';
            let openBraces = 1;
            while (i < len && openBraces > 0) {
                if (movesPart[i] === '{')
                    openBraces++;
                else if (movesPart[i] === '}')
                    openBraces--;
                if (openBraces > 0) {
                    comment += movesPart[i];
                }
                i++;
            }
            comment = comment.trim();
            if (comment && currentList.length > 0) {
                currentList[currentList.length - 1].comments.push(comment);
            }
            continue;
        }
        // NAGs (e.g. $1, $2, $4)
        if (char === '$') {
            i++;
            let nagStr = '$';
            while (i < len && /\d/.test(movesPart[i])) {
                nagStr += movesPart[i];
                i++;
            }
            if (currentList.length > 0) {
                currentList[currentList.length - 1].nags.push(nagStr);
            }
            continue;
        }
        // Start variation (
        if (char === '(') {
            i++;
            // We need to associate this variation with the last move of the parent sequence
            const parentList = currentList;
            if (parentList.length > 0) {
                const lastMove = parentList[parentList.length - 1];
                const newVarList = [];
                lastMove.variations.push(newVarList);
                stack.push(currentList);
                currentList = newVarList;
            }
            else {
                // Safe fallback if variation starts at beginning
                const newVarList = [];
                stack.push(currentList);
                currentList = newVarList;
            }
            continue;
        }
        // End variation )
        if (char === ')') {
            i++;
            if (stack.length > 1) {
                currentList = stack.pop();
            }
            continue;
        }
        // Capture standard moves (SAN notation)
        // SAN contains letters, digits, +, #, =, /, - (castling O-O, O-O-O)
        if (/[a-zA-Z0-9+#=/-]/.test(char)) {
            let san = '';
            while (i < len && /[a-zA-Z0-9+#=/-]/.test(movesPart[i])) {
                san += movesPart[i];
                i++;
            }
            // Ignore game results from the move list
            if (['1-0', '0-1', '1/2-1/2', '*'].includes(san)) {
                continue;
            }
            // Determine move count based on list size or estimate
            const moveIndex = currentList.length + 1;
            const moveNum = Math.ceil(moveIndex / 2);
            currentList.push({
                moveNumber: moveNum,
                san,
                comments: [],
                nags: [],
                variations: []
            });
            continue;
        }
        // If character doesn't fit, skip to avoid hangs
        i++;
    }
    // Find result in headers or moves
    let result = headers['Result'] || '*';
    if (result === '*') {
        const lastWord = movesPart.split(/\s+/).pop();
        if (lastWord && ['1-0', '0-1', '1/2-1/2', '*'].includes(lastWord)) {
            result = lastWord;
        }
    }
    return {
        headers,
        moves,
        result
    };
}
