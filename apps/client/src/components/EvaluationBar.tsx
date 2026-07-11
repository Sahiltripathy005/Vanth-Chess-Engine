import React from 'react';
import { useGameStore } from '../store/gameStore';

export const EvaluationBar: React.FC = () => {
  const { evalScore, isLoadingAnalysis, analysisReport } = useGameStore();

  // If score is centipawns, convert to pawn units
  const scoreInPawns = evalScore / 100.0;
  
  // Calculate percentage of white height
  // standard view: +8 is 100% white, -8 is 0% white, 0 is 50% white
  let percentage = 50;
  if (scoreInPawns > 8) percentage = 95;
  else if (scoreInPawns < -8) percentage = 5;
  else {
    percentage = 50 + (scoreInPawns / 8) * 45;
  }

  // Handle mate display
  let scoreText = scoreInPawns.toFixed(2);
  if (analysisReport && analysisReport.best_move) {
    if (Math.abs(evalScore) > 20000) {
      const movesToMate = Math.round((30000 - Math.abs(evalScore)));
      scoreText = `M${evalScore > 0 ? '+' : '-'}${movesToMate}`;
      percentage = evalScore > 0 ? 100 : 0;
    }
  }

  return (
    <div className="relative w-8 h-full bg-[#262626] border border-borderColor rounded-lg overflow-hidden flex flex-col justify-between shadow-lg select-none">
      {/* Black indicator */}
      <div 
        className="w-full bg-[#171717] transition-all duration-700 ease-out"
        style={{ height: `${100 - percentage}%` }}
      />

      {/* White indicator */}
      <div 
        className="w-full bg-[#f1f1f1] transition-all duration-700 ease-out"
        style={{ height: `${percentage}%` }}
      />

      {/* Numerical score label */}
      <div 
        className={`absolute inset-x-0 text-center font-mono font-bold text-[10px] py-1 transition-colors duration-700 ${
          percentage > 50 ? 'text-[#171717] bottom-1' : 'text-[#f1f1f1] top-1'
        }`}
      >
        {isLoadingAnalysis ? '...' : scoreText}
      </div>
    </div>
  );
};
