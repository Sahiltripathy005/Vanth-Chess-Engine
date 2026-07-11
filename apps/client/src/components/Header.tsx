import React, { useState } from 'react';
import { useGameStore } from '../store/gameStore';
import { RefreshCw, LayoutTemplate, Save, Copy, FilePlus } from 'lucide-react';

export const Header: React.FC = () => {
  const {
    currentFen,
    setFen,
    resetGame,
    flipBoard,
    saveCurrentPosition,
  } = useGameStore();

  const [fenInput, setFenInput] = useState('');
  const [showLoadModal, setShowLoadModal] = useState(false);
  const [showSaveModal, setShowSaveModal] = useState(false);
  const [positionName, setPositionName] = useState('');
  const [positionNotes, setPositionNotes] = useState('');
  const [copied, setCopied] = useState(false);

  const handleCopyFen = () => {
    navigator.clipboard.writeText(currentFen);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleLoadFen = () => {
    if (fenInput.trim()) {
      setFen(fenInput.trim());
      setShowLoadModal(false);
      setFenInput('');
    }
  };

  const handleSavePosition = async () => {
    if (positionName.trim()) {
      await saveCurrentPosition(positionName.trim(), positionNotes.trim());
      setShowSaveModal(false);
      setPositionName('');
      setPositionNotes('');
    }
  };

  return (
    <header className="flex items-center justify-between px-6 py-4 bg-panelBg border-b border-borderColor select-none">
      {/* Brand Logo */}
      <div className="flex items-center space-x-3">
        <div className="w-8 h-8 rounded bg-primary flex items-center justify-center font-bold text-white tracking-widest text-lg shadow-lg">
          V
        </div>
        <div>
          <h1 className="font-bold text-md tracking-wide">VANTh Chess Studio</h1>
          <span className="text-[10px] text-textMuted font-mono">Engine Version 2.1 • UI 3.0</span>
        </div>
      </div>

      {/* Center FEN Status */}
      <div className="hidden lg:flex items-center space-x-2 bg-accentBg border border-borderColor rounded-lg px-3 py-1.5 max-w-[450px]">
        <span className="text-[10px] text-textMuted uppercase font-semibold select-none font-mono">FEN:</span>
        <span className="text-xs font-mono truncate text-slate-300 select-all" title={currentFen}>{currentFen}</span>
        <button
          onClick={handleCopyFen}
          className="p-1 text-textMuted hover:text-white rounded transition-colors"
          title="Copy FEN"
        >
          <Copy size={12} className={copied ? 'text-secondary' : ''} />
        </button>
      </div>

      {/* Control Buttons */}
      <div className="flex items-center space-x-2">
        <button
          onClick={() => setShowLoadModal(true)}
          className="flex items-center space-x-1.5 px-3 py-1.5 text-xs bg-accentBg hover:bg-slate-700 transition-colors border border-borderColor rounded-lg text-slate-300 font-medium"
        >
          <FilePlus size={14} />
          <span>Load FEN</span>
        </button>
        
        <button
          onClick={() => setShowSaveModal(true)}
          className="flex items-center space-x-1.5 px-3 py-1.5 text-xs bg-accentBg hover:bg-slate-700 transition-colors border border-borderColor rounded-lg text-slate-300 font-medium"
        >
          <Save size={14} />
          <span>Save Position</span>
        </button>

        <div className="w-px h-6 bg-borderColor" />

        <button
          onClick={flipBoard}
          className="p-2 text-textMuted hover:text-white bg-accentBg border border-borderColor hover:bg-slate-700 rounded-lg transition-all"
          title="Flip Board"
        >
          <LayoutTemplate size={14} />
        </button>

        <button
          onClick={resetGame}
          className="p-2 text-textMuted hover:text-white bg-accentBg border border-borderColor hover:bg-slate-700 rounded-lg transition-all"
          title="Reset Position"
        >
          <RefreshCw size={14} />
        </button>
      </div>

      {/* Load FEN Dialog Overlay Modal */}
      {showLoadModal && (
        <div className="absolute inset-0 bg-black/60 backdrop-blur-sm z-50 flex items-center justify-center">
          <div className="bg-cardBg border border-borderColor rounded-xl p-6 w-[480px] shadow-2xl">
            <h3 className="text-md font-semibold mb-3">Load FEN String</h3>
            <textarea
              className="w-full bg-accentBg border border-borderColor rounded-lg p-3 text-xs font-mono text-slate-200 focus:outline-none focus:ring-1 focus:ring-primary h-[80px]"
              placeholder="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
              value={fenInput}
              onChange={(e) => setFenInput(e.target.value)}
            />
            <div className="flex justify-end space-x-2 mt-4">
              <button
                onClick={() => setShowLoadModal(false)}
                className="px-4 py-2 text-xs rounded-lg hover:bg-slate-800 text-textMuted transition-colors"
              >
                Cancel
              </button>
              <button
                onClick={handleLoadFen}
                className="px-4 py-2 text-xs rounded-lg bg-primary hover:bg-primaryLight text-white font-medium transition-colors"
              >
                Load Board
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Save Position Dialog Overlay Modal */}
      {showSaveModal && (
        <div className="absolute inset-0 bg-black/60 backdrop-blur-sm z-50 flex items-center justify-center">
          <div className="bg-cardBg border border-borderColor rounded-xl p-6 w-[400px] shadow-2xl">
            <h3 className="text-md font-semibold mb-3">Save Current Position</h3>
            
            <div className="space-y-4">
              <div>
                <label className="block text-[10px] text-textMuted uppercase font-bold tracking-wider mb-1">Name</label>
                <input
                  type="text"
                  className="w-full bg-accentBg border border-borderColor rounded-lg px-3 py-2 text-xs text-slate-200 focus:outline-none focus:ring-1 focus:ring-primary"
                  placeholder="Tactical pin puzzle"
                  value={positionName}
                  onChange={(e) => setPositionName(e.target.value)}
                />
              </div>

              <div>
                <label className="block text-[10px] text-textMuted uppercase font-bold tracking-wider mb-1">Notes (Optional)</label>
                <textarea
                  className="w-full bg-accentBg border border-borderColor rounded-lg p-3 text-xs text-slate-200 focus:outline-none focus:ring-1 focus:ring-primary h-[60px]"
                  placeholder="White has a forced mate in 3 here..."
                  value={positionNotes}
                  onChange={(e) => setPositionNotes(e.target.value)}
                />
              </div>
            </div>

            <div className="flex justify-end space-x-2 mt-6">
              <button
                onClick={() => setShowSaveModal(false)}
                className="px-4 py-2 text-xs rounded-lg hover:bg-slate-800 text-textMuted transition-colors"
              >
                Cancel
              </button>
              <button
                onClick={handleSavePosition}
                className="px-4 py-2 text-xs rounded-lg bg-primary hover:bg-primaryLight text-white font-medium transition-colors"
              >
                Save Position
              </button>
            </div>
          </div>
        </div>
      )}
    </header>
  );
};
