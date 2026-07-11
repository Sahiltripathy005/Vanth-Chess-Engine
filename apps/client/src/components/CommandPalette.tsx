import React, { useState, useEffect, useRef } from 'react';
import { useGameStore } from '../store/gameStore';
import { 
  Search, Compass, BookOpen, Settings as SettingsIcon, History as HistoryIcon,
  RotateCcw, RefreshCw, Upload, Save
} from 'lucide-react';

interface CommandPaletteProps {
  isOpen: boolean;
  onClose: () => void;
  setActivePanel: (panel: 'analysis' | 'moves' | 'history' | 'settings') => void;
}

export const CommandPalette: React.FC<CommandPaletteProps> = ({ isOpen, onClose, setActivePanel }) => {
  const { 
    runAnalysis, flipBoard, resetGame, saveCurrentPosition,
    undoMove, redoMove
  } = useGameStore();

  const [search, setSearch] = useState('');
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef<HTMLInputElement>(null);

  const commands = [
    {
      id: 'analyze',
      name: 'Run Engine Analysis',
      desc: 'Start Explainable AI search on the current position',
      icon: Compass,
      action: () => {
        runAnalysis();
        setActivePanel('analysis');
      }
    },
    {
      id: 'flip',
      name: 'Flip Chess Board',
      desc: 'Toggle the perspective between White and Black',
      icon: RefreshCw,
      action: () => flipBoard()
    },
    {
      id: 'reset',
      name: 'Reset Chess Board',
      desc: 'Clear the board and set pieces to the initial layout',
      icon: RotateCcw,
      action: () => resetGame()
    },
    {
      id: 'load_fen',
      name: 'Load FEN String',
      desc: 'Import custom position via FEN input',
      icon: Upload,
      action: () => {
        const fen = prompt('Enter FEN string:');
        if (fen) {
          useGameStore.getState().setFen(fen);
        }
      }
    },
    {
      id: 'save_pos',
      name: 'Save Current Position',
      desc: 'Save the current chess layout to your workspace',
      icon: Save,
      action: () => {
        const name = prompt('Enter name for this position:');
        if (name) {
          saveCurrentPosition(name);
          setActivePanel('history');
        }
      }
    },
    {
      id: 'go_analysis',
      name: 'Switch to Analysis Dashboard',
      desc: 'View tactical motifs and evaluations',
      icon: Compass,
      action: () => setActivePanel('analysis')
    },
    {
      id: 'go_tree',
      name: 'Switch to Move Tree',
      desc: 'Explore alternative game branches and lines',
      icon: BookOpen,
      action: () => setActivePanel('moves')
    },
    {
      id: 'go_history',
      name: 'Switch to Saved Positions & History',
      desc: 'Retrieve previous search records',
      icon: HistoryIcon,
      action: () => setActivePanel('history')
    },
    {
      id: 'go_settings',
      name: 'Switch to Engine Options',
      desc: 'Adjust depth, search time, and threads limit',
      icon: SettingsIcon,
      action: () => setActivePanel('settings')
    },
    {
      id: 'undo',
      name: 'Undo Last Move',
      desc: 'Go back one ply in the active line',
      icon: RotateCcw,
      action: () => undoMove()
    },
    {
      id: 'redo',
      name: 'Redo Next Move',
      desc: 'Advance forward one ply in the active line',
      icon: RefreshCw,
      action: () => redoMove()
    }
  ];

  const filteredCommands = commands.filter(cmd =>
    cmd.name.toLowerCase().includes(search.toLowerCase()) ||
    cmd.desc.toLowerCase().includes(search.toLowerCase())
  );

  useEffect(() => {
    if (isOpen) {
      setSearch('');
      setSelectedIndex(0);
      setTimeout(() => inputRef.current?.focus(), 50);
    }
  }, [isOpen]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (!isOpen) return;

      if (e.key === 'ArrowDown') {
        e.preventDefault();
        setSelectedIndex(prev => (prev + 1) % filteredCommands.length);
      } else if (e.key === 'ArrowUp') {
        e.preventDefault();
        setSelectedIndex(prev => (prev - 1 + filteredCommands.length) % filteredCommands.length);
      } else if (e.key === 'Enter') {
        e.preventDefault();
        if (filteredCommands[selectedIndex]) {
          filteredCommands[selectedIndex].action();
          onClose();
        }
      } else if (e.key === 'Escape') {
        e.preventDefault();
        onClose();
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isOpen, filteredCommands, selectedIndex, onClose]);

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-start justify-center pt-24 bg-black/60 backdrop-blur-sm select-none">
      <div 
        className="w-full max-w-lg bg-panelBg border border-borderColor rounded-xl shadow-2xl overflow-hidden animate-in fade-in zoom-in-95 duration-150"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Search header bar */}
        <div className="flex items-center px-4 py-3 border-b border-borderColor bg-cardBg">
          <Search size={18} className="text-textMuted mr-3" />
          <input
            ref={inputRef}
            type="text"
            placeholder="Type a command or action to execute..."
            value={search}
            onChange={(e) => {
              setSearch(e.target.value);
              setSelectedIndex(0);
            }}
            className="flex-1 bg-transparent text-sm text-slate-100 placeholder-slate-500 focus:outline-none"
          />
          <kbd className="text-[10px] bg-accentBg px-2 py-0.5 rounded text-textMuted font-mono">ESC</kbd>
        </div>

        {/* Command list */}
        <div className="max-h-[300px] overflow-y-auto p-2 space-y-0.5 scrollbar-thin">
          {filteredCommands.length === 0 ? (
            <div className="p-4 text-center text-textMuted text-xs">No matching commands found.</div>
          ) : (
            filteredCommands.map((cmd, idx) => {
              const Icon = cmd.icon;
              const isSelected = idx === selectedIndex;
              return (
                <div
                  key={cmd.id}
                  onClick={() => {
                    cmd.action();
                    onClose();
                  }}
                  className={`flex items-center px-3 py-2.5 rounded-lg cursor-pointer transition-colors ${
                    isSelected 
                      ? 'bg-primary text-white' 
                      : 'hover:bg-accentBg/60 text-slate-300'
                  }`}
                >
                  <Icon size={16} className={`mr-3 ${isSelected ? 'text-white' : 'text-textMuted'}`} />
                  <div className="flex-1 min-w-0">
                    <div className="text-xs font-semibold truncate">{cmd.name}</div>
                    <div className={`text-[10px] truncate ${isSelected ? 'text-white/80' : 'text-textMuted'}`}>
                      {cmd.desc}
                    </div>
                  </div>
                </div>
              );
            })
          )}
        </div>

        {/* Shortcut hint footer */}
        <div className="flex items-center justify-between px-4 py-2 border-t border-borderColor/60 bg-cardBg/30 text-[10px] text-textMuted font-mono">
          <div className="flex space-x-3">
            <span>↑↓ Navigate</span>
            <span>Enter Select</span>
          </div>
          <span>Vanth Studio Copilot</span>
        </div>
      </div>
    </div>
  );
};
