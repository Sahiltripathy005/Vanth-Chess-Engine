import React, { useEffect, useState } from 'react';
import { useGameStore } from '../store/gameStore';
import { History, Bookmark, Trash2, Calendar, ChevronRight } from 'lucide-react';

export const HistoryPanel: React.FC = () => {
  const {
    savedPositions,
    analysisHistoryList,
    fetchSavedPositions,
    fetchAnalysisHistoryList,
    deleteSavedPosition,
    setFen,
  } = useGameStore();

  const [activeTab, setActiveTab] = useState<'positions' | 'analyses'>('positions');

  useEffect(() => {
    fetchSavedPositions();
    fetchAnalysisHistoryList();
  }, [fetchSavedPositions, fetchAnalysisHistoryList]);

  const handleSelectPosition = (fen: string) => {
    setFen(fen);
  };

  const handleSelectHistoryAnalysis = (item: any) => {
    // Set FEN
    setFen(item.fen);
    
    // Proactively load the historical JSON report into store to view old analysis details instantly!
    useGameStore.setState({
      analysisReport: item.report,
      evalScore: item.score,
    });
  };

  return (
    <div className="flex flex-col h-full bg-panelBg border-l border-borderColor overflow-hidden select-none">
      {/* Tabs */}
      <div className="flex border-b border-borderColor bg-cardBg text-xs">
        <button
          onClick={() => setActiveTab('positions')}
          className={`flex-1 py-3 flex items-center justify-center space-x-1.5 font-semibold transition-all border-b-2 cursor-pointer ${
            activeTab === 'positions'
              ? 'border-primary text-white bg-accentBg/10'
              : 'border-transparent text-textMuted hover:text-slate-200'
          }`}
        >
          <Bookmark size={13} />
          <span>Saved Layouts</span>
        </button>
        <button
          onClick={() => setActiveTab('analyses')}
          className={`flex-1 py-3 flex items-center justify-center space-x-1.5 font-semibold transition-all border-b-2 cursor-pointer ${
            activeTab === 'analyses'
              ? 'border-primary text-white bg-accentBg/10'
              : 'border-transparent text-textMuted hover:text-slate-200'
          }`}
        >
          <History size={13} />
          <span>Recent Runs</span>
        </button>
      </div>

      {/* Main List */}
      <div className="flex-1 overflow-y-auto p-3 space-y-2">
        {activeTab === 'positions' ? (
          savedPositions.length === 0 ? (
            <div className="text-center text-textMuted py-8 italic text-xs">
              No saved positions. Use "Save Position" in header.
            </div>
          ) : (
            savedPositions.map((pos) => (
              <div
                key={pos.id}
                className="group relative bg-cardBg border border-borderColor hover:border-slate-600 rounded-xl p-3 text-xs flex flex-col justify-between transition-all"
              >
                <div 
                  className="cursor-pointer pr-8"
                  onClick={() => handleSelectPosition(pos.fen)}
                >
                  <h4 className="font-semibold text-slate-200">{pos.name}</h4>
                  {pos.notes && <p className="text-[10px] text-textMuted mt-0.5 line-clamp-2">{pos.notes}</p>}
                  <span className="text-[9px] font-mono text-textMuted block mt-1.5 truncate">{pos.fen}</span>
                </div>

                <button
                  onClick={() => deleteSavedPosition(pos.id)}
                  className="absolute top-3 right-3 p-1.5 text-textMuted hover:text-danger hover:bg-danger/10 rounded-lg transition-colors"
                  title="Delete Layout"
                >
                  <Trash2 size={13} />
                </button>
              </div>
            ))
          )
        ) : (
          analysisHistoryList.length === 0 ? (
            <div className="text-center text-textMuted py-8 italic text-xs">
              No previous runs. Make sure to run "Run Analysis" on positions.
            </div>
          ) : (
            analysisHistoryList.map((item) => {
              const dateStr = new Date(item.createdAt).toLocaleDateString(undefined, {
                month: 'short',
                day: 'numeric',
                hour: '2-digit',
                minute: '2-digit'
              });

              return (
                <div
                  key={item.id}
                  onClick={() => handleSelectHistoryAnalysis(item)}
                  className="flex items-center justify-between bg-cardBg border border-borderColor hover:border-slate-600 rounded-xl p-3 text-xs cursor-pointer transition-all hover:bg-accentBg/10 group"
                >
                  <div className="space-y-1 pr-4 truncate">
                    <div className="flex items-center space-x-2">
                      <span className="font-mono font-bold text-slate-200">{(item.score / 100.0).toFixed(2)}</span>
                      <span className="text-[10px] font-mono text-textMuted">best: {item.bestMove}</span>
                      <span className="text-[9px] bg-accentBg px-1 rounded text-textMuted">d:{item.depth}</span>
                    </div>
                    <span className="text-[9px] font-mono text-textMuted block truncate">{item.fen}</span>
                    <div className="flex items-center space-x-1 text-[8px] text-textMuted">
                      <Calendar size={10} />
                      <span>{dateStr}</span>
                    </div>
                  </div>
                  
                  <ChevronRight size={14} className="text-textMuted group-hover:text-slate-300 transition-colors" />
                </div>
              );
            })
          )
        )}
      </div>
    </div>
  );
};
