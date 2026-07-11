export interface ChessPiece {
  type: 'p' | 'n' | 'b' | 'r' | 'q' | 'k';
  color: 'w' | 'b';
}

export type BoardArray = (ChessPiece | null)[][];

export const START_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

export function parseFen(fen: string): BoardArray {
  const parts = fen.split(' ');
  const rows = parts[0].split('/');
  const board: BoardArray = [];

  for (let r = 0; r < 8; r++) {
    const row: (ChessPiece | null)[] = [];
    const rowStr = rows[r];
    
    for (let c = 0; c < rowStr.length; c++) {
      const char = rowStr[c];
      if (/[1-8]/.test(char)) {
        const emptyCount = parseInt(char, 10);
        for (let e = 0; e < emptyCount; e++) {
          row.push(null);
        }
      } else {
        const color = char === char.toUpperCase() ? 'w' : 'b';
        const type = char.toLowerCase() as any;
        row.push({ type, color });
      }
    }
    board.push(row);
  }
  return board;
}

export function toSquareName(row: number, col: number): string {
  const files = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'];
  const ranks = ['8', '7', '6', '5', '4', '3', '2', '1'];
  return `${files[col]}${ranks[row]}`;
}

export function fromSquareName(name: string): { row: number; col: number } {
  const file = name.charCodeAt(0) - 'a'.charCodeAt(0);
  const rank = 8 - parseInt(name[1], 10);
  return { row: rank, col: file };
}

// Check if a move is standard pawn push, capture, or promotion
export function isMovePromotion(fen: string, from: string, to: string): boolean {
  const board = parseFen(fen);
  const { row: fromRow, col: fromCol } = fromSquareName(from);
  const piece = board[fromRow][fromCol];
  if (!piece || piece.type !== 'p') return false;

  const { row: toRow } = fromSquareName(to);
  if (piece.color === 'w' && toRow === 0) return true;
  if (piece.color === 'b' && toRow === 7) return true;
  return false;
}
