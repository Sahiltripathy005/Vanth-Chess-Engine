import React, { useEffect, useState } from 'react';
import { useGameStore } from './store/gameStore';
import { Header } from './components/Header';
import { ChessBoard } from './components/ChessBoard';
import { EvaluationBar } from './components/EvaluationBar';
import { AnalysisPanel } from './components/AnalysisPanel';
import { MoveList } from './components/MoveList';
import { SettingsPanel } from './components/SettingsPanel';
import { HistoryPanel } from './components/HistoryPanel';
import { EvaluationGraph } from './components/EvaluationGraph';
import { PositionInspector } from './components/PositionInspector';
import { CommandPalette } from './components/CommandPalette';
import { GameIntelligence } from './components/GameIntelligence';
import { Compass, BookOpen, Settings as SettingsIcon, History as HistoryIcon, HelpCircle, Keyboard, Sparkles } from 'lucide-react';

export const App: React.FC = () => {
  const { 
    fetchSettings, 
    fetchAvailableLegalMoves, 
    undoMove, 
    redoMove, 
    runAnalysis, 
    cancelAnalysis,
    isLiveSearching,
    saveCurrentPosition
  } = useGameStore();

  const [activePanel, setActivePanel] = useState<'analysis' | 'gameReport' | 'moves' | 'history' | 'settings'>('analysis');
  const [isPaletteOpen, setIsPaletteOpen] = useState(false);
  const [isHelpOpen, setIsHelpOpen] = useState(false);

  useEffect(() => {
    fetchSettings();
    fetchAvailableLegalMoves();
  }, [fetchSettings, fetchAvailableLegalMoves]);

  // Global Keyboard Shortcuts handler
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Avoid hotkeys when typing in input components
      const target = e.target as HTMLElement;
      if (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.isContentEditable) {
        return;
      }

      // Ctrl + K -> Command Palette
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') {
        e.preventDefault();
        setIsPaletteOpen(prev => !prev);
      }

      // ? (Shift + /) -> Keyboard Help
      if (e.key === '?' || (e.shiftKey && e.key === '/')) {
        e.preventDefault();
        setIsHelpOpen(prev => !prev);
      }

      // Space -> Toggle Search
      if (e.key === ' ') {
        e.preventDefault();
        if (isLiveSearching) {
          cancelAnalysis();
        } else {
          runAnalysis();
        }
      }

      // Left Arrow -> Undo
      if (e.key === 'ArrowLeft') {
        e.preventDefault();
        undoMove();
      }

      // Right Arrow -> Redo
      if (e.key === 'ArrowRight') {
        e.preventDefault();
        redoMove();
      }

      // Ctrl + Z -> Undo
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'z' && !e.shiftKey) {
        e.preventDefault();
        undoMove();
      }

      // Ctrl + Shift + Z -> Redo
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'z' && e.shiftKey) {
        e.preventDefault();
        redoMove();
      }

      // Ctrl + S -> Save Position
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') {
        e.preventDefault();
        const name = prompt('Enter a name for this position:');
        if (name) {
          saveCurrentPosition(name);
          setActivePanel('history');
        }
      }

      // Ctrl + L -> Load FEN
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'l') {
        e.preventDefault();
        const fen = prompt('Enter FEN string to load:');
        if (fen) {
          useGameStore.getState().setFen(fen);
        }
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [isLiveSearching, undoMove, redoMove, runAnalysis, cancelAnalysis, saveCurrentPosition]);

  return (
    <div className="flex h-screen w-screen overflow-hidden bg-darkBg text-textColor font-sans select-none">
      
      {/* 1. Left Icon Navigation Sidebar */}
      <aside className="w-[60px] bg-panelBg border-r border-borderColor flex flex-col items-center py-4 justify-between shrink-0 select-none">
        <div className="flex flex-col items-center space-y-4 w-full">
          {/* Logo Badge */}
          <div className="w-8 h-8 rounded-lg bg-indigo-600 flex items-center justify-center font-bold text-white shadow-lg text-sm select-none">
            VN
          </div>
          
          <div className="w-8 h-px bg-borderColor" />
          
          {/* Action Tabs */}
          {[
            { id: 'analysis', icon: Compass, label: 'Explore & Analyze' },
            { id: 'gameReport', icon: Sparkles, label: 'Game Intelligence' },
            { id: 'moves', icon: BookOpen, label: 'Move Records' },
            { id: 'history', icon: HistoryIcon, label: 'Saved & History' },
            { id: 'settings', icon: SettingsIcon, label: 'Preferences' },
          ].map((tab) => {
            const Icon = tab.icon;
            const isActive = activePanel === tab.id;
            return (
              <button
                key={tab.id}
                onClick={() => setActivePanel(tab.id as any)}
                className={`p-2.5 rounded-xl transition-all cursor-pointer ${
                  isActive
                    ? 'bg-primary text-white shadow-md'
                    : 'text-textMuted hover:text-slate-200 hover:bg-accentBg/50'
                }`}
                title={tab.label}
              >
                <Icon size={18} />
              </button>
            );
          })}
        </div>

        <div className="flex flex-col items-center space-y-3">
          <button
            onClick={() => setIsHelpOpen(true)}
            className="p-2 rounded-xl text-textMuted hover:text-white hover:bg-slate-800 transition-all cursor-pointer"
            title="Keyboard Shortcuts"
          >
            <Keyboard size={16} />
          </button>
          <span className="text-[8px] font-mono text-textMuted select-none">v3.2.0</span>
        </div>
      </aside>

      {activePanel === 'gameReport' ? (
        <div className="flex-1 h-full overflow-hidden flex flex-col bg-darkBg">
          <Header />
          <GameIntelligence />
        </div>
      ) : (
        <>
          {/* 2. Main Chess Board Panel */}
          <main className="flex-1 flex flex-col h-full overflow-hidden">
            <Header />
            
            {/* Main interactive area */}
            <div className="flex-1 flex flex-col p-6 space-y-4 overflow-y-auto scrollbar-thin">
              <div className="flex items-stretch justify-center space-x-6 h-[460px] shrink-0">
                <EvaluationBar />
                <div className="flex-1 max-w-[460px] h-full">
                  <ChessBoard />
                </div>
                
                {/* Position summary inline inside board layout */}
                <div className="w-[300px]">
                  <PositionInspector />
                </div>
              </div>

              {/* Bottom Evaluation graph timeline */}
              <div className="w-full max-w-[840px] mx-auto pt-2 shrink-0">
                <EvaluationGraph />
              </div>
            </div>
          </main>

          {/* 3. Right Analysis and Detail Panel */}
          <section className="w-[380px] h-full shrink-0 border-l border-borderColor">
            {activePanel === 'analysis' && <AnalysisPanel />}
            {activePanel === 'moves' && <MoveList />}
            {activePanel === 'history' && <HistoryPanel />}
            {activePanel === 'settings' && <SettingsPanel />}
          </section>
        </>
      )}

      {/* 4. Global Command Palette & Dialog Overlays */}
      <CommandPalette
        isOpen={isPaletteOpen}
        onClose={() => setIsPaletteOpen(false)}
        setActivePanel={setActivePanel}
      />

      {/* Keyboard Shortcuts Help Dialog */}
      {isHelpOpen && (
        <div 
          className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm select-none"
          onClick={() => setIsHelpOpen(false)}
        >
          <div 
            className="w-full max-w-sm bg-panelBg border border-borderColor rounded-2xl shadow-2xl p-5 space-y-4"
            onClick={(e) => e.stopPropagation()}
          >
            <div className="flex items-center space-x-2 border-b border-borderColor/60 pb-3">
              <HelpCircle className="text-primary" size={20} />
              <h3 className="font-bold text-sm text-slate-100">Keyboard Shortcuts</h3>
            </div>

            <div className="space-y-2.5 text-xs">
              {[
                { keys: ['Ctrl', 'K'], desc: 'Open Command Palette' },
                { keys: ['Space'], desc: 'Toggle Engine Analysis' },
                { keys: ['←', '→'], desc: 'Navigate played moves history' },
                { keys: ['Ctrl', 'Z'], desc: 'Undo move' },
                { keys: ['Ctrl', 'Shift', 'Z'], desc: 'Redo move' },
                { keys: ['Ctrl', 'S'], desc: 'Save current position' },
                { keys: ['Ctrl', 'L'], desc: 'Load FEN layout' },
                { keys: ['?'], desc: 'Toggle this help menu' }
              ].map((s, idx) => (
                <div key={idx} className="flex items-center justify-between">
                  <span className="text-slate-300">{s.desc}</span>
                  <div className="flex space-x-1">
                    {s.keys.map((k, kIdx) => (
                      <kbd key={kIdx} className="bg-accentBg px-1.5 py-0.5 border border-borderColor rounded text-[10px] text-slate-200 font-mono shadow-sm">
                        {k}
                      </kbd>
                    ))}
                  </div>
                </div>
              ))}
            </div>

            <button
              onClick={() => setIsHelpOpen(false)}
              className="w-full py-2 bg-accentBg hover:bg-slate-700 text-slate-200 border border-borderColor rounded-xl text-xs font-semibold transition-colors cursor-pointer"
            >
              Dismiss
            </button>
          </div>
        </div>
      )}

    </div>
  );
};

export default App;
