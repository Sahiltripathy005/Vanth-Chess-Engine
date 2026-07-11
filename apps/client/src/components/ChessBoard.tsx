import React, { useState } from 'react';
import { useGameStore } from '../store/gameStore';
import { parseFen, toSquareName, isMovePromotion } from '../utils/chess';
import { ChessPieceSvg } from './ChessPieceSvg';

export const ChessBoard: React.FC = () => {
  const {
    currentFen,
    flipped,
    selectedSquare,
    legalMoves,
    boardTheme,
    selectSquare,
    playMove,
  } = useGameStore();

  const [promoModal, setPromoModal] = useState<{ from: string; to: string } | null>(null);

  const board = parseFen(currentFen);
  
  // Set up board coordinate layouts
  const rows = [0, 1, 2, 3, 4, 5, 6, 7];
  const cols = [0, 1, 2, 3, 4, 5, 6, 7];

  const displayRows = flipped ? [...rows].reverse() : rows;
  const displayCols = flipped ? [...cols].reverse() : cols;

  // Board themes
  const themes = {
    emerald: { light: 'bg-[#eeeed2] text-[#769656]', dark: 'bg-[#769656] text-[#eeeed2]' },
    wood: { light: 'bg-[#f0d9b5] text-[#b58863]', dark: 'bg-[#b58863] text-[#f0d9b5]' },
    slate: { light: 'bg-[#e2e8f0] text-[#475569]', dark: 'bg-[#475569] text-[#e2e8f0]' },
    classic: { light: 'bg-[#f0f0f0] text-[#a0a0a0]', dark: 'bg-[#a0a0a0] text-[#f0f0f0]' },
  };

  const currentTheme = themes[boardTheme] || themes.emerald;

  const handleSquareClick = async (square: string) => {
    if (selectedSquare && legalMoves.includes(square)) {
      // Check for pawn promotion
      if (isMovePromotion(currentFen, selectedSquare, square)) {
        setPromoModal({ from: selectedSquare, to: square });
      } else {
        await playMove(selectedSquare, square);
      }
    } else {
      selectSquare(square);
    }
  };

  const handlePromotionSelect = async (pieceChar: string) => {
    if (promoModal) {
      await playMove(promoModal.from, promoModal.to, pieceChar);
      setPromoModal(null);
    }
  };

  return (
    <div className="relative w-full max-w-[560px] aspect-square rounded-lg overflow-hidden border border-borderColor shadow-2xl bg-panelBg">
      <div className="grid grid-cols-8 grid-rows-8 w-full h-full">
        {displayRows.map((r) =>
          displayCols.map((c) => {
            const squareName = toSquareName(r, c);
            const isDark = (r + c) % 2 === 1;
            const piece = board[r][c];
            
            const isSelected = selectedSquare === squareName;
            const isLegalTarget = legalMoves.includes(squareName);

            // Determine border coordinates labels positioning
            const showFileLabel = r === (flipped ? 0 : 7);
            const showRankLabel = c === (flipped ? 7 : 0);

            const squareColorClass = isDark ? currentTheme.dark : currentTheme.light;
            const selectedClass = isSelected ? 'ring-4 ring-indigo-500 ring-inset opacity-90' : '';
            const targetDotClass = isLegalTarget && !piece ? 'after:content-[""] after:w-4 after:h-4 after:bg-indigo-600/60 after:rounded-full after:absolute after:top-1/2 after:left-1/2 after:-translate-x-1/2 after:-translate-y-1/2' : '';
            const targetRingClass = isLegalTarget && piece ? 'ring-4 ring-red-500/70 ring-inset' : '';

            return (
              <div
                key={squareName}
                className={`chess-square relative select-none flex items-center justify-center transition-all ${squareColorClass} ${selectedClass} ${targetDotClass} ${targetRingClass}`}
                onClick={() => handleSquareClick(squareName)}
              >
                {piece && (
                  <div className="chess-piece">
                    <ChessPieceSvg type={piece.type} color={piece.color} />
                  </div>
                )}

                {/* Coordinate Labels */}
                {showFileLabel && (
                  <span className={`absolute bottom-0.5 right-1 text-[10px] font-bold select-none ${isDark ? 'text-textColor/30' : 'text-darkBg/30'}`}>
                    {squareName[0]}
                  </span>
                )}
                {showRankLabel && (
                  <span className={`absolute top-0.5 left-1 text-[10px] font-bold select-none ${isDark ? 'text-textColor/30' : 'text-darkBg/30'}`}>
                    {squareName[1]}
                  </span>
                )}
              </div>
            );
          })
        )}
      </div>

      {/* Promotion Dialog Modal Overlay */}
      {promoModal && (
        <div className="absolute inset-0 bg-black/60 backdrop-blur-sm z-50 flex items-center justify-center">
          <div className="bg-cardBg border border-borderColor rounded-xl p-6 w-[280px] shadow-2xl flex flex-col items-center">
            <h3 className="text-sm font-semibold mb-4 text-center">Promote Pawn To</h3>
            <div className="grid grid-cols-4 gap-3 w-full">
              {[
                { type: 'q', label: 'Queen' },
                { type: 'r', label: 'Rook' },
                { type: 'b', label: 'Bishop' },
                { type: 'n', label: 'Knight' },
              ].map((p) => (
                <button
                  key={p.type}
                  onClick={() => handlePromotionSelect(p.type)}
                  className="flex flex-col items-center justify-center p-2 rounded-lg bg-accentBg hover:bg-indigo-600 transition-colors border border-borderColor"
                >
                  <div className="w-10 h-10">
                    {/* Promoting piece has active player color */}
                    <ChessPieceSvg type={p.type as any} color={currentFen.split(' ')[1] as any} />
                  </div>
                  <span className="text-[10px] mt-1 text-textMuted">{p.label}</span>
                </button>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
