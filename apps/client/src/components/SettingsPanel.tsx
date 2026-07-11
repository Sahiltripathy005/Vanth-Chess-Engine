import React from 'react';
import { useGameStore } from '../store/gameStore';
import { Settings } from 'lucide-react';

export const SettingsPanel: React.FC = () => {
  const {
    settings,
    boardTheme,
    pieceTheme,
    updateSettings,
  } = useGameStore();

  const handleBoardThemeChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    updateSettings({ boardTheme: e.target.value as any });
  };

  const handlePieceThemeChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    updateSettings({ pieceTheme: e.target.value as any });
  };

  const handleHashChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    updateSettings({ hash: parseInt(e.target.value, 10) });
  };

  const handleThreadsChange = (e: React.ChangeEvent<HTMLSelectElement>) => {
    updateSettings({ threads: parseInt(e.target.value, 10) });
  };

  const handleOwnBookToggle = (e: React.ChangeEvent<HTMLInputElement>) => {
    updateSettings({ ownBook: e.target.checked });
  };

  const handlePonderToggle = (e: React.ChangeEvent<HTMLInputElement>) => {
    updateSettings({ ponder: e.target.checked });
  };

  return (
    <div className="flex flex-col h-full bg-panelBg border-l border-borderColor overflow-hidden select-none">
      <div className="flex items-center space-x-2 p-4 border-b border-borderColor bg-cardBg">
        <Settings size={16} className="text-primary" />
        <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Engine & Customization Settings</span>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-6 text-xs text-slate-300">
        
        {/* 1. Interface customization */}
        <div className="space-y-3">
          <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Appearance</h3>
          
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-[10px] text-textMuted mb-1">Board Color</label>
              <select
                value={boardTheme}
                onChange={handleBoardThemeChange}
                className="w-full bg-accentBg border border-borderColor rounded-lg px-2.5 py-1.5 focus:outline-none focus:ring-1 focus:ring-primary cursor-pointer text-slate-200"
              >
                <option value="emerald">Emerald Green</option>
                <option value="wood">Walnut Wood</option>
                <option value="slate">Slate Gray</option>
                <option value="classic">Light Classic</option>
              </select>
            </div>

            <div>
              <label className="block text-[10px] text-textMuted mb-1">Piece Style</label>
              <select
                value={pieceTheme}
                onChange={handlePieceThemeChange}
                className="w-full bg-accentBg border border-borderColor rounded-lg px-2.5 py-1.5 focus:outline-none focus:ring-1 focus:ring-primary cursor-pointer text-slate-200"
              >
                <option value="neo">Neo-Modern</option>
                <option value="classic">Traditional</option>
              </select>
            </div>
          </div>
        </div>

        <div className="w-full h-px bg-borderColor" />

        {/* 2. Engine configuration */}
        <div className="space-y-4">
          <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Engine Performance</h3>
          
          <div className="space-y-3">
            <div>
              <label className="block text-[10px] text-textMuted mb-1">Transposition Table (Hash)</label>
              <select
                value={settings.hash}
                onChange={handleHashChange}
                className="w-full bg-accentBg border border-borderColor rounded-lg px-2.5 py-1.5 focus:outline-none focus:ring-1 focus:ring-primary cursor-pointer text-slate-200"
              >
                <option value={16}>16 MB</option>
                <option value={32}>32 MB</option>
                <option value={64}>64 MB</option>
                <option value={128}>128 MB</option>
              </select>
            </div>

            <div>
              <label className="block text-[10px] text-textMuted mb-1">CPU Threads</label>
              <select
                value={settings.threads}
                onChange={handleThreadsChange}
                className="w-full bg-accentBg border border-borderColor rounded-lg px-2.5 py-1.5 focus:outline-none focus:ring-1 focus:ring-primary cursor-pointer text-slate-200"
              >
                <option value={1}>1 Core (Default)</option>
                <option value={2}>2 Cores</option>
                <option value={4}>4 Cores</option>
                <option value={8}>8 Cores</option>
              </select>
            </div>
          </div>
        </div>

        <div className="w-full h-px bg-borderColor" />

        {/* 3. Logic options */}
        <div className="space-y-3">
          <h3 className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Analysis Prefs</h3>
          
          <div className="space-y-2">
            <label className="flex items-center space-x-3 cursor-pointer p-2 bg-accentBg/30 hover:bg-accentBg/50 rounded-lg border border-borderColor/40 transition-colors">
              <input
                type="checkbox"
                checked={settings.ownBook}
                onChange={handleOwnBookToggle}
                className="rounded border-borderColor bg-accentBg text-primary focus:ring-primary focus:ring-offset-darkBg"
              />
              <div>
                <div className="font-semibold text-slate-200">Use Opening Book</div>
                <div className="text-[10px] text-textMuted">Speeds up opening moves with built-in library</div>
              </div>
            </label>

            <label className="flex items-center space-x-3 cursor-pointer p-2 bg-accentBg/30 hover:bg-accentBg/50 rounded-lg border border-borderColor/40 transition-colors">
              <input
                type="checkbox"
                checked={settings.ponder}
                onChange={handlePonderToggle}
                className="rounded border-borderColor bg-accentBg text-primary focus:ring-primary focus:ring-offset-darkBg"
              />
              <div>
                <div className="font-semibold text-slate-200">Pondering (Thinking ahead)</div>
                <div className="text-[10px] text-textMuted">Let engine analyze opponent's potential replies</div>
              </div>
            </label>
          </div>
        </div>

      </div>
    </div>
  );
};
