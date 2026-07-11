import React, { useState } from 'react';
import { useGameStore } from '../store/gameStore';
import { 
  Play, Loader2, Info, AlertTriangle, Shield, Compass, Zap, Move, Grid, XCircle 
} from 'lucide-react';

interface ReasonCardProps {
  reason: string;
}

const ReasonCard: React.FC<ReasonCardProps> = ({ reason }) => {
  const [expanded, setExpanded] = useState(false);

  let category = 'Tactical';
  let colorClass = 'border-indigo-500/30 bg-indigo-500/5 text-indigo-400';
  let Icon = Compass;
  let principle = 'Vanth analyzes deep tactical chains, piece sacrifices, and position-specific motifs to secure a robust edge.';

  if (reason.toLowerCase().includes('develop')) {
    category = 'Development';
    colorClass = 'border-blue-500/30 bg-blue-500/5 text-blue-400';
    Icon = Zap;
    principle = 'Developing your minor pieces (knights and bishops) early is key to securing king safety, controlling center space, and coordinating attacks.';
  } else if (reason.toLowerCase().includes('center') || reason.toLowerCase().includes('central')) {
    category = 'Center Control';
    colorClass = 'border-emerald-500/30 bg-emerald-500/5 text-emerald-400';
    Icon = Grid;
    principle = 'Controlling the central squares allows your pieces to move fluidly between flanks and clamps down on enemy coordination.';
  } else if (reason.toLowerCase().includes('mobility') || reason.toLowerCase().includes('space') || reason.toLowerCase().includes('territorial')) {
    category = 'Mobility & Space';
    colorClass = 'border-purple-500/30 bg-purple-500/5 text-purple-400';
    Icon = Move;
    principle = 'Expanse of movement restricts enemy counter-play and creates weak squares in the opponent\'s territory that your pieces can exploit.';
  } else if (reason.toLowerCase().includes('king') || reason.toLowerCase().includes('safety')) {
    category = 'King Safety';
    colorClass = 'border-amber-500/30 bg-amber-500/5 text-amber-400';
    Icon = Shield;
    principle = 'Keeping your king well-defended behind a solid pawn shield stops immediate mating tactical threats and ensures peace of mind.';
  }

  return (
    <div
      onClick={() => setExpanded(!expanded)}
      className={`border rounded-xl p-3 cursor-pointer transition-all duration-300 hover:border-slate-500 ${colorClass}`}
    >
      <div className="flex items-start justify-between">
        <div className="flex items-center space-x-2">
          <div className="p-1 rounded-lg bg-white/5">
            <Icon size={14} />
          </div>
          <span className="text-[10px] font-bold uppercase tracking-wider opacity-90">{category}</span>
        </div>
        <span className="text-[9px] bg-white/10 px-1.5 py-0.5 rounded font-semibold">
          High Confidence
        </span>
      </div>
      <p className="text-xs font-semibold text-slate-200 mt-2 leading-relaxed">{reason}</p>
      
      {expanded && (
        <div className="mt-2.5 pt-2.5 border-t border-white/10 text-[10px] text-slate-300 leading-normal italic">
          <strong>Strategic Concept:</strong> {principle}
        </div>
      )}
    </div>
  );
};

export const AnalysisPanel: React.FC = () => {
  const {
    analysisReport,
    isLoadingAnalysis,
    isLiveSearching,
    liveAnalysisInfo,
    runAnalysis,
    cancelAnalysis,
    settings,
    updateSettings,
  } = useGameStore();

  const handleDepthChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    updateSettings({ depth: parseInt(e.target.value, 10) });
  };

  const getSubscoreBarWidth = (val: number) => {
    const absVal = Math.min(Math.abs(val), 300);
    return `${(absVal / 300) * 100}%`;
  };

  return (
    <div className="flex flex-col h-full bg-panelBg overflow-hidden select-none">
      {/* Top Header controls */}
      <div className="flex items-center justify-between p-4 border-b border-borderColor bg-cardBg">
        <div className="flex items-center space-x-2">
          <label className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Depth:</label>
          <select
            value={settings.depth}
            onChange={handleDepthChange}
            disabled={isLoadingAnalysis}
            className="bg-accentBg border border-borderColor text-xs rounded px-2 py-1 text-slate-200 focus:outline-none focus:ring-1 focus:ring-primary cursor-pointer"
          >
            {[1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map((d) => (
              <option key={d} value={d}>
                {d} ply
              </option>
            ))}
          </select>
        </div>

        {isLiveSearching ? (
          <button
            onClick={cancelAnalysis}
            className="flex items-center space-x-1.5 px-3 py-1.5 bg-danger hover:bg-red-500 text-white font-medium text-xs rounded-lg shadow-lg transition-colors cursor-pointer"
          >
            <XCircle size={14} />
            <span>Cancel</span>
          </button>
        ) : (
          <button
            onClick={runAnalysis}
            disabled={isLoadingAnalysis}
            className="flex items-center space-x-1.5 px-3 py-1.5 bg-primary hover:bg-primaryLight disabled:bg-indigo-600/50 text-white font-medium text-xs rounded-lg shadow-lg transition-colors cursor-pointer"
          >
            {isLoadingAnalysis ? (
              <>
                <Loader2 className="animate-spin" size={14} />
                <span>Running...</span>
              </>
            ) : (
              <>
                <Play fill="white" size={12} />
                <span>Run Analysis</span>
              </>
            )}
          </button>
        )}
      </div>

      {/* Main content scroll area */}
      <div className="flex-1 overflow-y-auto p-4 space-y-5 scrollbar-thin">
        
        {/* 1. Live Engine Search metrics (Engine Inspector) */}
        {isLiveSearching && liveAnalysisInfo && (
          <div className="bg-cardBg border border-borderColor rounded-xl p-4 space-y-3.5 shadow-md">
            <div className="flex items-center justify-between border-b border-borderColor/60 pb-2">
              <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider flex items-center">
                <Loader2 className="animate-spin mr-1.5 text-primary" size={12} />
                Engine Inspector (Thinking...)
              </span>
              <span className="text-[10px] font-mono text-secondary font-semibold bg-secondary/10 px-2 py-0.5 rounded">
                Score: {liveAnalysisInfo.scoreType === 'cp' ? (liveAnalysisInfo.score / 100.0).toFixed(2) : `Mate in ${liveAnalysisInfo.score}`}
              </span>
            </div>

            <div className="grid grid-cols-2 gap-3 text-[11px] font-mono text-textMuted">
              <div className="bg-accentBg/40 border border-borderColor/30 rounded-lg p-2">
                Depth: <span className="text-slate-100 font-semibold">{liveAnalysisInfo.depth} ply</span>
              </div>
              <div className="bg-accentBg/40 border border-borderColor/30 rounded-lg p-2">
                NPS: <span className="text-slate-100 font-semibold">{liveAnalysisInfo.nps.toLocaleString()}</span>
              </div>
              <div className="bg-accentBg/40 border border-borderColor/30 rounded-lg p-2">
                Nodes: <span className="text-slate-100 font-semibold">{liveAnalysisInfo.nodes.toLocaleString()}</span>
              </div>
              <div className="bg-accentBg/40 border border-borderColor/30 rounded-lg p-2">
                Time: <span className="text-slate-100 font-semibold">{(liveAnalysisInfo.timeMs / 1000.0).toFixed(2)}s</span>
              </div>
            </div>

            {/* Live PV line */}
            {liveAnalysisInfo.pv && liveAnalysisInfo.pv.length > 0 && (
              <div className="pt-2 border-t border-borderColor/60">
                <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider block mb-1">Principal Variation</span>
                <div className="flex flex-wrap gap-1 font-mono text-xs text-slate-300">
                  {liveAnalysisInfo.pv.map((m, idx) => (
                    <span key={idx} className="bg-accentBg px-1.5 py-0.5 rounded border border-borderColor/40">
                      {idx % 2 === 0 ? `${Math.floor(idx / 2) + 1}.` : ''} {m}
                    </span>
                  ))}
                </div>
              </div>
            )}
          </div>
        )}

        {!analysisReport && !isLiveSearching ? (
          <div className="flex flex-col items-center justify-center h-[200px] text-center p-4">
            <Info size={32} className="text-textMuted mb-2 opacity-50" />
            <h3 className="text-sm font-semibold text-slate-300">No Analysis Loaded</h3>
            <p className="text-xs text-textMuted mt-1 max-w-[200px]">
              Click "Run Analysis" or press "Space" to start live search.
            </p>
          </div>
        ) : (
          analysisReport && (
            <>
              {/* 2. Best Move recommendation */}
              <div className="bg-cardBg border border-borderColor rounded-xl p-4 shadow-sm">
                <div className="flex items-center justify-between">
                  <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Engine Recommendation</span>
                  <span className="text-[10px] font-mono text-secondary font-semibold bg-secondary/10 px-2 py-0.5 rounded">
                    Score: {(analysisReport.evaluation / 100.0).toFixed(2)}
                  </span>
                </div>
                <h2 className="text-3xl font-extrabold font-mono mt-1 text-white">{analysisReport.best_move}</h2>
                
                {/* Final PV list */}
                {analysisReport.alternatives && analysisReport.alternatives.length > 0 && (
                  <div className="mt-3 pt-3 border-t border-borderColor/60">
                    <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider block mb-1">Principal Variation</span>
                    <div className="flex flex-wrap gap-1 font-mono text-xs text-slate-300">
                      {analysisReport.alternatives[0]?.pv.map((m, idx) => (
                        <span key={idx} className="bg-accentBg px-1.5 py-0.5 rounded border border-borderColor/40">
                          {idx % 2 === 0 ? `${Math.floor(idx / 2) + 1}.` : ''} {m}
                        </span>
                      ))}
                    </div>
                  </div>
                )}
              </div>

              {/* 3. Move Explanations (Explainable cards) */}
              {analysisReport.explanation && analysisReport.explanation.reasons.length > 0 && (
                <div className="space-y-2">
                  <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Move Justifications</h3>
                  <div className="space-y-2">
                    {analysisReport.explanation.reasons.map((r, idx) => (
                      <ReasonCard key={idx} reason={r} />
                    ))}
                  </div>
                </div>
              )}

              {/* 4. Static Evaluation breakdown */}
              {analysisReport.breakdown && (
                <div className="space-y-2">
                  <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Static Evaluation Breakdown</h3>
                  <div className="bg-cardBg border border-borderColor rounded-xl p-4 space-y-3 font-mono text-xs">
                    {Object.entries(analysisReport.breakdown)
                      .filter(([key]) => key !== 'total')
                      .map(([key, val]) => {
                        const isPositive = val >= 0;
                        return (
                          <div key={key} className="flex flex-col space-y-1">
                            <div className="flex items-center justify-between text-[11px]">
                              <span className="capitalize text-slate-300">{key.replace('_', ' ')}</span>
                              <span className={val > 0 ? 'text-secondary' : val < 0 ? 'text-danger' : 'text-textMuted'}>
                                {val > 0 ? `+${val}` : val}
                              </span>
                            </div>
                            
                            <div className="relative w-full h-1.5 bg-accentBg rounded-full overflow-hidden">
                              <div
                                className={`absolute h-full top-0 rounded-full transition-all duration-500 ${
                                  isPositive ? 'left-1/2 bg-secondary' : 'right-1/2 bg-danger'
                                }`}
                                style={{ width: getSubscoreBarWidth(val) }}
                              />
                            </div>
                          </div>
                        );
                      })}

                    <div className="pt-2 border-t border-borderColor/60 flex items-center justify-between text-xs font-bold">
                      <span className="text-slate-200">Total Static Score</span>
                      <span className={analysisReport.breakdown.total > 0 ? 'text-secondary' : analysisReport.breakdown.total < 0 ? 'text-danger' : 'text-slate-300'}>
                        {analysisReport.breakdown.total > 0 ? `+${analysisReport.breakdown.total}` : analysisReport.breakdown.total} cp
                      </span>
                    </div>
                  </div>
                </div>
              )}

              {/* 5. Tactical themes motifs */}
              {analysisReport.tactical_themes && (
                <div className="space-y-2">
                  <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Tactical Motifs</h3>
                  <div className="flex flex-wrap gap-1.5">
                    {Object.entries(analysisReport.tactical_themes)
                      .filter(([key, val]) => key !== 'descriptions' && val === true)
                      .map(([key]) => (
                        <span key={key} className="px-2 py-0.5 bg-indigo-600/10 text-indigo-400 border border-indigo-500/20 text-[10px] font-semibold uppercase rounded-full">
                          {key.replace('_', ' ')}
                        </span>
                      ))}
                    
                    {analysisReport.tactical_themes.descriptions.map((desc, idx) => (
                      <div key={idx} className="flex items-center space-x-1.5 w-full bg-accentBg/40 border border-borderColor/40 rounded-lg p-2 text-xs text-textMuted">
                        <AlertTriangle size={12} className="text-indigo-400" />
                        <span>{desc}</span>
                      </div>
                    ))}

                    {Object.entries(analysisReport.tactical_themes)
                      .filter(([key, val]) => key !== 'descriptions' && val === true).length === 0 && (
                        <span className="text-xs text-textMuted italic">No active tactical motifs.</span>
                      )}
                  </div>
                </div>
              )}

              {/* 6. Alternative Candidate moves */}
              {analysisReport.alternatives && analysisReport.alternatives.length > 1 && (
                <div className="space-y-2">
                  <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Alternative Candidates</h3>
                  <div className="space-y-1.5">
                    {analysisReport.alternatives.slice(1).map((alt, idx) => {
                      const diff = ((analysisReport.evaluation - alt.score) / 100.0).toFixed(2);
                      return (
                        <div key={idx} className="flex items-center justify-between bg-accentBg border border-borderColor rounded-lg p-2.5 font-mono text-xs">
                          <div className="flex items-center space-x-2">
                            <span className="text-textMuted">#{idx + 2}</span>
                            <span className="font-bold text-white">{alt.move}</span>
                            <span className="text-[10px] text-textMuted">({(alt.score / 100.0).toFixed(2)})</span>
                          </div>
                          <span className="text-[10px] font-semibold text-danger">
                            -{diff} cp delta
                          </span>
                        </div>
                      );
                    })}
                  </div>
                </div>
              )}

              {/* 7. Search diagnostics and execution details */}
              {analysisReport.search_stats && (
                <div className="pt-2 border-t border-borderColor/60 space-y-2">
                  <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Search Diagnostics</h3>
                  <div className="grid grid-cols-2 gap-2 text-[10px] font-mono text-textMuted bg-cardBg border border-borderColor rounded-xl p-3">
                    <div>Nodes: <span className="text-slate-300 font-semibold">{analysisReport.search_stats.nodes.toLocaleString()}</span></div>
                    <div>NPS: <span className="text-slate-300 font-semibold">{analysisReport.search_stats.nps.toLocaleString()}</span></div>
                    <div>Time: <span className="text-slate-300 font-semibold">{analysisReport.search_stats.time_ms} ms</span></div>
                    <div>TT Hits: <span className="text-slate-300 font-semibold">{analysisReport.search_stats.tt_hits.toLocaleString()}</span></div>
                  </div>
                </div>
              )}
            </>
          )
        )}
      </div>
    </div>
  );
};
