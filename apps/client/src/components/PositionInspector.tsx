import React from 'react';
import { useGameStore } from '../store/gameStore';
import { Shield, Sparkles, Award, LayoutGrid, Info } from 'lucide-react';

export const PositionInspector: React.FC = () => {
  const { analysisReport, isLoadingAnalysis } = useGameStore();

  if (isLoadingAnalysis) {
    return (
      <div className="bg-cardBg border border-borderColor rounded-xl p-6 text-center select-none">
        <div className="animate-pulse flex flex-col items-center space-y-3">
          <div className="h-6 w-32 bg-slate-700 rounded" />
          <div className="h-16 w-full bg-slate-700 rounded-xl" />
          <div className="h-20 w-full bg-slate-700 rounded-xl" />
        </div>
      </div>
    );
  }

  if (!analysisReport) {
    return (
      <div className="bg-cardBg border border-borderColor rounded-xl p-6 text-center text-textMuted select-none flex flex-col items-center justify-center">
        <Info size={24} className="mb-2 opacity-50" />
        <span className="text-xs">Run analysis to generate a detailed strategic summary.</span>
      </div>
    );
  }

  const { breakdown, classifications } = analysisReport;

  // Determine active phase
  let activePhase = 'Middlegame';
  if (classifications?.opening) activePhase = 'Opening';
  else if (classifications?.endgame) activePhase = 'Endgame';

  // Format material score
  const materialScore = breakdown?.material ?? 0;
  const isWhiteMaterialAhead = materialScore > 0;

  return (
    <div className="bg-cardBg border border-borderColor rounded-xl p-4 space-y-4 shadow-md select-none">
      <div className="flex items-center justify-between border-b border-borderColor/60 pb-2">
        <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Position Summary</span>
        <span className="text-[10px] bg-primary/20 text-primary border border-primary/30 px-2 py-0.5 rounded font-bold uppercase">
          {activePhase} Phase
        </span>
      </div>

      {/* Game Phase timeline indicator */}
      <div className="space-y-1.5">
        <div className="flex justify-between text-[10px] text-textMuted font-mono">
          <span className={activePhase === 'Opening' ? 'text-white font-bold' : ''}>Opening</span>
          <span className={activePhase === 'Middlegame' ? 'text-white font-bold' : ''}>Middlegame</span>
          <span className={activePhase === 'Endgame' ? 'text-white font-bold' : ''}>Endgame</span>
        </div>
        <div className="w-full h-1.5 bg-accentBg rounded-full overflow-hidden flex">
          <div className={`h-full flex-1 border-r border-panelBg ${activePhase === 'Opening' ? 'bg-primary' : 'bg-slate-700'}`} />
          <div className={`h-full flex-1 border-r border-panelBg ${activePhase === 'Middlegame' ? 'bg-primary' : 'bg-slate-700'}`} />
          <div className={`h-full flex-1 ${activePhase === 'Endgame' ? 'bg-primary' : 'bg-slate-700'}`} />
        </div>
      </div>

      {/* Material & Strategic balances */}
      <div className="grid grid-cols-2 gap-3 text-xs">
        {/* Material Balance Card */}
        <div className="bg-accentBg/30 border border-borderColor/40 rounded-lg p-3 space-y-1.5">
          <div className="flex items-center justify-between text-[10px] text-textMuted uppercase font-bold tracking-wider">
            <span>Material Advantage</span>
          </div>
          <div className="font-mono text-sm font-extrabold text-slate-100">
            {materialScore === 0 ? (
              <span className="text-slate-400">Equal Material</span>
            ) : isWhiteMaterialAhead ? (
              <span className="text-secondary">White (+{(materialScore / 100).toFixed(1)})</span>
            ) : (
              <span className="text-danger">Black ({(materialScore / 100).toFixed(1)})</span>
            )}
          </div>
        </div>

        {/* Space Advantage Card */}
        <div className="bg-accentBg/30 border border-borderColor/40 rounded-lg p-3 space-y-1.5">
          <div className="flex items-center justify-between text-[10px] text-textMuted uppercase font-bold tracking-wider">
            <span>Space Advantage</span>
          </div>
          <div className="font-mono text-sm font-extrabold text-slate-100">
            {(breakdown?.space ?? 0) === 0 ? (
              <span className="text-slate-400">Balanced</span>
            ) : (breakdown?.space ?? 0) > 0 ? (
              <span className="text-secondary">White Space</span>
            ) : (
              <span className="text-danger">Black Space</span>
            )}
          </div>
        </div>
      </div>

      {/* King safety & structure meters */}
      <div className="space-y-3 font-mono text-[11px]">
        {/* King Safety Row */}
        <div className="flex items-center justify-between">
          <div className="flex items-center text-slate-300">
            <Shield size={12} className="mr-1 text-slate-400" />
            <span>King Safety Balance</span>
          </div>
          <span className={(breakdown?.king_safety ?? 0) >= 0 ? 'text-secondary' : 'text-danger'}>
            {(breakdown?.king_safety ?? 0) >= 0 ? `+${breakdown?.king_safety}` : breakdown?.king_safety} cp
          </span>
        </div>

        {/* Pawn Structure Row */}
        <div className="flex items-center justify-between">
          <div className="flex items-center text-slate-300">
            <LayoutGrid size={12} className="mr-1 text-slate-400" />
            <span>Pawn Structure Strength</span>
          </div>
          <span className={(breakdown?.pawn_structure ?? 0) >= 0 ? 'text-secondary' : 'text-danger'}>
            {(breakdown?.pawn_structure ?? 0) >= 0 ? `+${breakdown?.pawn_structure}` : breakdown?.pawn_structure} cp
          </span>
        </div>

        {/* Tactical Complexity Row */}
        <div className="flex items-center justify-between">
          <div className="flex items-center text-slate-300">
            <Sparkles size={12} className="mr-1 text-slate-400" />
            <span>Tactical Threat level</span>
          </div>
          <span className={(breakdown?.threats ?? 0) >= 0 ? 'text-secondary' : 'text-danger'}>
            {(breakdown?.threats ?? 0) >= 0 ? `+${breakdown?.threats}` : breakdown?.threats} cp
          </span>
        </div>
      </div>

      {/* Strategic Tags */}
      {classifications?.labels && classifications.labels.length > 0 && (
        <div className="flex flex-wrap gap-1.5 pt-2 border-t border-borderColor/60">
          {classifications.labels.map((lbl, idx) => (
            <span
              key={idx}
              className="px-2 py-0.5 bg-primary/10 text-primary border border-primary/20 rounded-md text-[9px] font-bold uppercase tracking-wider flex items-center"
            >
              <Award size={10} className="mr-1" />
              {lbl}
            </span>
          ))}
        </div>
      )}
    </div>
  );
};
