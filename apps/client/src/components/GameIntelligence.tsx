import React, { useState, useEffect, useRef } from 'react';
import { Play, Pause, ChevronLeft, ChevronRight, ChevronsLeft, ChevronsRight, Upload, FileText, Download, CheckCircle, AlertTriangle, RefreshCw } from 'lucide-react';
import { parseFen } from '../utils/chess';
import { ChessPieceSvg } from './ChessPieceSvg';

interface GameMoveRecord {
  moveIndex: number;
  san: string;
  uci: string;
  fen: string;
  score: number;
  delta: number;
  classification: string;
  bestMove?: string;
  tacticalMistake?: string;
  positionalMistake?: string;
  explanation?: string;
  breakdown?: any;
  tacticalThemes?: string[];
  comments?: string[];
}

interface GameReport {
  accuracyWhite: number;
  accuracyBlack: number;
  whiteMistakes: number;
  blackMistakes: number;
  whiteBlunders: number;
  blackBlunders: number;
  whiteBrilliants: number;
  blackBrilliants: number;
  opening: string;
  endgame: string;
  trainingAdvice: string;
  strengths: string[];
  weaknesses: string[];
}

interface GameData {
  id: string;
  pgn: string;
  result: string;
  whitePlayer: string;
  blackPlayer: string;
  date: string;
  accuracyWhite: number | null;
  accuracyBlack: number | null;
  eco: string | null;
  openingName: string | null;
  analysisStatus: string;
  headers: Record<string, string>;
  moves: GameMoveRecord[];
  report: GameReport | null;
}

export const GameIntelligence: React.FC = () => {
  const [pgnInput, setPgnInput] = useState('');
  const [recentGames, setRecentGames] = useState<any[]>([]);
  const [activeGame, setActiveGame] = useState<GameData | null>(null);
  const [currentIndex, setCurrentIndex] = useState(0); // 0 = start position FEN
  const [isPlaying, setIsPlaying] = useState(false);
  const [progress, setProgress] = useState<number | null>(null);
  const [isImporting, setIsImporting] = useState(false);

  const eventSourceRef = useRef<EventSource | null>(null);
  const playTimerRef = useRef<any>(null);

  // Load recent games
  const fetchRecentGames = async () => {
    try {
      const res = await fetch('http://localhost:4000/api/games');
      const data = await res.json();
      setRecentGames(data);
    } catch (e) {
      console.error(e);
    }
  };

  useEffect(() => {
    fetchRecentGames();
    return () => {
      if (eventSourceRef.current) eventSourceRef.current.close();
      if (playTimerRef.current) clearInterval(playTimerRef.current);
    };
  }, []);

  // Handle PGN Submission
  const handleImport = async (pgnText?: string) => {
    const text = pgnText || pgnInput;
    if (!text.trim()) return;

    setIsImporting(true);
    setProgress(0);
    try {
      const res = await fetch('http://localhost:4000/api/games/import', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pgn: text })
      });
      const game = await res.json();
      if (game.error) throw new Error(game.error);

      // Start SSE listening for progress updates
      setupSseListener(game.id);
    } catch (err) {
      alert(`Import Failed: ${err}`);
      setIsImporting(false);
      setProgress(null);
    }
  };

  // Setup SSE stream listener
  const setupSseListener = (gameId: string) => {
    if (eventSourceRef.current) eventSourceRef.current.close();

    const es = new EventSource(`http://localhost:4000/api/games/${gameId}/stream`);
    eventSourceRef.current = es;

    es.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === 'progress') {
        setProgress(data.progress);
      } else if (data.type === 'completed' || data.type === 'failed') {
        es.close();
        setIsImporting(false);
        setProgress(null);
        // Load the finalized game
        loadGame(gameId);
        fetchRecentGames();
      }
    };

    es.onerror = () => {
      es.close();
      setIsImporting(false);
      setProgress(null);
      loadGame(gameId);
    };
  };

  // Load single game
  const loadGame = async (gameId: string) => {
    try {
      const res = await fetch(`http://localhost:4000/api/games/${gameId}`);
      const data: GameData = await res.json();
      setActiveGame(data);
      setCurrentIndex(0); // reset to starting position FEN
      setIsPlaying(false);
    } catch (e) {
      console.error(e);
    }
  };

  // Autoplay moves loop
  useEffect(() => {
    if (isPlaying && activeGame) {
      playTimerRef.current = setInterval(() => {
        setCurrentIndex((prev) => {
          if (prev >= activeGame.moves.length) {
            setIsPlaying(false);
            return prev;
          }
          return prev + 1;
        });
      }, 1000);
    } else {
      if (playTimerRef.current) clearInterval(playTimerRef.current);
    }
    return () => {
      if (playTimerRef.current) clearInterval(playTimerRef.current);
    };
  }, [isPlaying, activeGame]);

  // Export report
  const handleExport = (format: 'markdown' | 'json') => {
    if (!activeGame) return;
    window.open(`http://localhost:4000/api/games/${activeGame.id}/export?format=${format}`, '_blank');
  };

  // Renders the board from FEN
  const renderMiniBoard = () => {
    // Determine FEN based on current move index
    // index 0 -> starting position FEN
    // index i -> FEN after move i
    let fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
    if (activeGame && currentIndex > 0 && activeGame.moves[currentIndex - 1]) {
      fen = activeGame.moves[currentIndex - 1].fen;
    }

    const board = parseFen(fen);
    const rows = [0, 1, 2, 3, 4, 5, 6, 7];
    const cols = [0, 1, 2, 3, 4, 5, 6, 7];

    return (
      <div className="w-full aspect-square grid grid-cols-8 grid-rows-8 rounded-lg overflow-hidden border border-borderColor/60 shadow-xl bg-panelBg">
        {rows.map((r) =>
          cols.map((c) => {
            const isDark = (r + c) % 2 === 1;
            const piece = board[r][c];
            const bgClass = isDark ? 'bg-[#769656]' : 'bg-[#eeeed2]';

            return (
              <div key={`${r}-${c}`} className={`relative flex items-center justify-center ${bgClass}`}>
                {piece && (
                  <div className="w-[85%] h-[85%]">
                    <ChessPieceSvg type={piece.type} color={piece.color} />
                  </div>
                )}
              </div>
            );
          })
        )}
      </div>
    );
  };

  const getClassificationColor = (cls: string) => {
    switch (cls) {
      case 'Brilliant': return 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/40';
      case 'Best': return 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/40';
      case 'Excellent': return 'bg-teal-500/20 text-teal-400 border border-teal-500/30';
      case 'Book': return 'bg-blue-500/20 text-blue-400 border border-blue-500/30';
      case 'Good': return 'bg-slate-500/20 text-slate-300 border border-slate-600/30';
      case 'Inaccuracy': return 'bg-yellow-500/20 text-yellow-400 border border-yellow-500/30';
      case 'Mistake': return 'bg-orange-500/20 text-orange-400 border border-orange-500/40';
      case 'Blunder': return 'bg-red-500/20 text-red-400 border border-red-500/40';
      case 'Missed Win': return 'bg-rose-500/20 text-rose-400 border border-rose-500/40';
      default: return 'bg-slate-700/20 text-slate-400';
    }
  };

  // Render SVG area-based evaluation graph
  const renderEvaluationGraph = () => {
    if (!activeGame || activeGame.moves.length === 0) return null;

    const scores = activeGame.moves.map(m => {
      // Clamp evaluation between -1000cp (-10) and +1000cp (+10)
      const val = m.score;
      return Math.max(-1000, Math.min(1000, val));
    });

    const width = 500;
    const height = 100;
    const padding = 10;

    const points = scores.map((score, i) => {
      const x = padding + (i / (scores.length - 1)) * (width - 2 * padding);
      // Center (0 score) is height / 2. Upwards is positive (White), downwards is negative (Black).
      const y = height / 2 - (score / 1000) * (height / 2 - padding);
      return `${x},${y}`;
    });

    const baselineY = height / 2;
    const pathData = `M ${padding},${baselineY} ` + points.map((p) => `L ${p}`).join(' ') + ` L ${width - padding},${baselineY} Z`;

    return (
      <div className="w-full bg-accentBg/40 border border-borderColor/45 rounded-xl p-3 space-y-1">
        <div className="flex items-center justify-between text-[10px] text-textMuted font-mono">
          <span>White Advantage (+10.0)</span>
          <span>Equal (0.0)</span>
          <span>Black Advantage (-10.0)</span>
        </div>
        <div className="relative">
          <svg viewBox={`0 0 ${width} ${height}`} className="w-full h-16 overflow-visible">
            {/* Zero line */}
            <line x1={padding} y1={baselineY} x2={width - padding} y2={baselineY} stroke="rgba(255,255,255,0.15)" strokeDasharray="3,3" />
            
            {/* Fill Area */}
            <path d={pathData} fill="rgba(99, 102, 241, 0.15)" stroke="none" />
            
            {/* Line Path */}
            <path
              d={points.map((p, i) => (i === 0 ? `M ${p}` : `L ${p}`)).join(' ')}
              fill="none"
              stroke="#6366f1"
              strokeWidth="2"
            />

            {/* Current Position Dot */}
            {currentIndex > 0 && currentIndex <= scores.length && (
              <circle
                cx={padding + ((currentIndex - 1) / (scores.length - 1)) * (width - 2 * padding)}
                cy={height / 2 - (scores[currentIndex - 1] / 1000) * (height / 2 - padding)}
                r="4"
                fill="#38bdf8"
                className="animate-pulse"
              />
            )}
          </svg>
        </div>
      </div>
    );
  };

  const samplePgn = `[Event "Vanth Engine Demo Match"]
[Site "Workspace Arena"]
[Date "2026.07.10"]
[Round "1"]
[White "Giri, Anish"]
[Black "Carlsen, Magnus"]
[Result "0-1"]

1. e4 c5 2. Nf3 d6 3. d4 cxd4 4. Nxd4 Nf6 5. Nc3 a6 6. Be3 e5 7. Nb3 Be6 8. Qd2 Nbd7 9. O-O-O Be7 10. f3 h5 11. Kb1 Rc8 12. Bd3 b5 13. h3 h4 14. f4 O-O 15. f5 Bc4 16. Qf2 b4 17. Ne2 d5 18. exd5 Bxd5 19. Bxa6 Ra8 20. Bb5 Qc7 21. Nec1 Rfb8 22. Bxd7 Qxd7 23. g4 hxg3 24. Qxg3 Qxf5 25. Rhg1 g6 26. Rdf1 Qe6 27. h4 Be4 28. h5 Nxh5 29. Qh2 Rc8 30. Rf2 Ng7 31. Re1 Nf5 32. Bc5 Bxc5 33. Nxc5 Rxc5 34. Rxe4 Rd8 35. Nd3 Rcd5 36. b3 Rxd3 37. cxd3 Rxd3 38. Qxe5 Rd1+ 39. Kc2 Qd7 40. Qe8+ Kg7 41. Qxd7 Rxd7 42. Rxb4 Ne3+ 43. Kc1 Rd1+ 44. Kb2 Rd7 45. Re2 Nd1+ 46. Kc2 f5 47. a4 g5 48. a5 f4 49. a6 Ne3+ 50. Kb1 Kf6 51. Rb7 Rd6 52. a7 Ra6 53. Ra2 Rxa7 54. Raxa7 g4 55. Rb5 g3 56. Ra6+ Ke7 57. Rb7+ Kd8 58. Ra8# 0-1`;

  return (
    <div className="flex h-[calc(100vh-64px)] w-full text-textColor font-sans select-none overflow-hidden bg-darkBg">
      {/* Sidebar: Recent game import / selection */}
      <aside className="w-[280px] bg-panelBg border-r border-borderColor flex flex-col h-full shrink-0">
        <div className="p-4 border-b border-borderColor/60">
          <h3 className="font-bold text-sm text-slate-100 flex items-center space-x-2">
            <Upload size={16} className="text-primary" />
            <span>Import PGN File</span>
          </h3>
          <p className="text-[10px] text-textMuted mt-1">Paste standard chess PGN text and initiate professional analysis.</p>
        </div>

        {/* Text Area for PGN */}
        <div className="p-4 flex-1 flex flex-col space-y-3 min-h-0">
          <textarea
            value={pgnInput}
            onChange={(e) => setPgnInput(e.target.value)}
            placeholder="Paste PGN here..."
            className="w-full flex-1 bg-accentBg text-xs p-2.5 rounded-xl border border-borderColor focus:outline-none focus:border-primary/60 resize-none font-mono text-slate-300 placeholder-slate-600"
          />

          <div className="flex space-x-2">
            <button
              onClick={() => handleImport()}
              disabled={isImporting}
              className="flex-1 py-2 bg-primary hover:bg-indigo-500 disabled:bg-primary/50 text-white rounded-xl text-xs font-semibold flex items-center justify-center space-x-1.5 cursor-pointer shadow-lg shadow-indigo-600/10"
            >
              {isImporting ? <RefreshCw size={13} className="animate-spin" /> : <CheckCircle size={13} />}
              <span>{isImporting ? `Analyzing (${progress || 0}%)` : 'Analyze PGN'}</span>
            </button>
            <button
              onClick={() => setPgnInput(samplePgn)}
              className="px-2.5 py-2 bg-accentBg hover:bg-slate-700 text-slate-300 rounded-xl text-xs border border-borderColor cursor-pointer"
              title="Load Demo PGN"
            >
              Demo
            </button>
          </div>
        </div>

        {/* Recent Games List */}
        <div className="h-[220px] border-t border-borderColor/60 flex flex-col min-h-0 bg-accentBg/10">
          <div className="p-3 border-b border-borderColor/40 flex items-center justify-between">
            <span className="text-[10px] font-bold tracking-wider text-textMuted uppercase">Recent Analysis Reports</span>
            <button onClick={fetchRecentGames} className="p-1 hover:bg-accentBg rounded text-textMuted hover:text-slate-200">
              <RefreshCw size={10} />
            </button>
          </div>

          <div className="flex-1 overflow-y-auto p-2 space-y-1.5 scrollbar-thin">
            {recentGames.map((game) => (
              <button
                key={game.id}
                onClick={() => loadGame(game.id)}
                className={`w-full p-2 rounded-xl text-left border transition-all text-xs flex flex-col space-y-1 ${
                  activeGame?.id === game.id
                    ? 'bg-primary/10 border-primary/40 text-slate-100'
                    : 'bg-accentBg/30 border-borderColor/40 hover:bg-accentBg/60 text-slate-300'
                }`}
              >
                <div className="flex items-center justify-between w-full">
                  <span className="font-semibold truncate max-w-[160px]">
                    {game.whitePlayer} vs {game.blackPlayer}
                  </span>
                  <span className="text-[9px] px-1 py-0.5 rounded bg-accentBg font-mono text-textMuted">
                    {game.result}
                  </span>
                </div>
                <div className="flex items-center justify-between text-[9px] text-textMuted">
                  <span>{game.date}</span>
                  <span className={game.analysisStatus === 'completed' ? 'text-emerald-400' : 'text-yellow-500'}>
                    {game.analysisStatus === 'completed' ? 'Analyzed' : 'Processing'}
                  </span>
                </div>
              </button>
            ))}
            {recentGames.length === 0 && (
              <div className="text-center py-6 text-xs text-textMuted">No reports found. Import a game to start!</div>
            )}
          </div>
        </div>
      </aside>

      {/* Main Workspace: Board Replay, Eval Graph */}
      <main className="flex-1 flex flex-col h-full bg-darkBg overflow-y-auto scrollbar-thin p-6">
        {activeGame ? (
          <div className="grid grid-cols-12 gap-6 items-start max-w-5xl mx-auto w-full">
            
            {/* Left: Interactive Replay & Board */}
            <div className="col-span-6 space-y-5">
              
              {/* Game Headers Card */}
              <div className="bg-panelBg/80 backdrop-blur border border-borderColor/50 rounded-2xl p-4 shadow-xl flex items-center justify-between">
                <div className="space-y-1">
                  <div className="text-xs text-textMuted font-medium">{activeGame.headers['Event'] || 'AI Analysis Session'}</div>
                  <h2 className="font-bold text-sm text-slate-100">
                    {activeGame.whitePlayer} <span className="text-primary font-normal">vs</span> {activeGame.blackPlayer}
                  </h2>
                  <div className="text-[10px] text-textMuted flex items-center space-x-2">
                    <span>Date: {activeGame.date}</span>
                    <span>•</span>
                    <span>Result: {activeGame.result}</span>
                  </div>
                </div>

                <div className="flex flex-col items-end space-y-1.5">
                  <span className="text-[10px] px-2 py-0.5 bg-indigo-500/10 text-indigo-400 border border-indigo-500/20 rounded-full font-mono">
                    ECO: {activeGame.eco || 'Book'}
                  </span>
                  <span className="text-[9px] text-textMuted truncate max-w-[120px]">
                    {activeGame.openingName || 'Unknown Opening'}
                  </span>
                </div>
              </div>

              {/* Chessboard Box */}
              <div className="space-y-3">
                {renderMiniBoard()}

                {/* Play Timeline Actions */}
                <div className="flex items-center justify-between bg-panelBg border border-borderColor rounded-xl p-2 px-3 shadow-md">
                  <div className="flex items-center space-x-2">
                    <button
                      onClick={() => setCurrentIndex(0)}
                      className="p-1.5 hover:bg-accentBg text-slate-300 hover:text-white rounded-lg transition-colors cursor-pointer"
                      title="First Position"
                    >
                      <ChevronsLeft size={16} />
                    </button>
                    <button
                      onClick={() => setCurrentIndex((prev) => Math.max(0, prev - 1))}
                      className="p-1.5 hover:bg-accentBg text-slate-300 hover:text-white rounded-lg transition-colors cursor-pointer"
                      title="Previous Move"
                    >
                      <ChevronLeft size={16} />
                    </button>
                    <button
                      onClick={() => setIsPlaying(!isPlaying)}
                      className="p-2 bg-primary hover:bg-indigo-500 text-white rounded-xl transition-colors cursor-pointer flex items-center justify-center"
                      title={isPlaying ? 'Pause Autoplay' : 'Autoplay Game'}
                    >
                      {isPlaying ? <Pause size={14} /> : <Play size={14} />}
                    </button>
                    <button
                      onClick={() => setCurrentIndex((prev) => Math.min(activeGame.moves.length, prev + 1))}
                      className="p-1.5 hover:bg-accentBg text-slate-300 hover:text-white rounded-lg transition-colors cursor-pointer"
                      title="Next Move"
                    >
                      <ChevronRight size={16} />
                    </button>
                    <button
                      onClick={() => setCurrentIndex(activeGame.moves.length)}
                      className="p-1.5 hover:bg-accentBg text-slate-300 hover:text-white rounded-lg transition-colors cursor-pointer"
                      title="Last Position"
                    >
                      <ChevronsRight size={16} />
                    </button>
                  </div>

                  {/* Move Index Display / Slider */}
                  <div className="flex items-center space-x-3">
                    <span className="text-[11px] font-mono text-textMuted select-none">
                      Move {currentIndex} / {activeGame.moves.length}
                    </span>
                    <input
                      type="range"
                      min="0"
                      max={activeGame.moves.length}
                      value={currentIndex}
                      onChange={(e) => setCurrentIndex(parseInt(e.target.value))}
                      className="w-24 h-1 bg-accentBg rounded-lg appearance-none cursor-pointer accent-primary"
                    />
                  </div>
                </div>
              </div>

              {/* Area Graph */}
              {renderEvaluationGraph()}

              {/* Export Panel */}
              <div className="flex space-x-3">
                <button
                  onClick={() => handleExport('markdown')}
                  className="flex-1 py-2 border border-borderColor hover:bg-accentBg text-slate-300 rounded-xl text-xs font-semibold flex items-center justify-center space-x-1.5 cursor-pointer"
                >
                  <FileText size={13} />
                  <span>Export Markdown Report</span>
                </button>
                <button
                  onClick={() => handleExport('json')}
                  className="flex-1 py-2 border border-borderColor hover:bg-accentBg text-slate-300 rounded-xl text-xs font-semibold flex items-center justify-center space-x-1.5 cursor-pointer"
                >
                  <Download size={13} />
                  <span>Export JSON Report</span>
                </button>
              </div>

            </div>

            {/* Right: Accuracy Metrics & Timeline Logs */}
            <div className="col-span-6 space-y-5">
              
              {/* Accuracy Dials */}
              {activeGame.report && (
                <div className="bg-panelBg/80 border border-borderColor/50 rounded-2xl p-4 shadow-xl grid grid-cols-2 gap-4">
                  {/* White Accuracy */}
                  <div className="flex flex-col items-center space-y-2 border-r border-borderColor/40">
                    <span className="text-[10px] text-textMuted font-bold uppercase tracking-wider">White Accuracy</span>
                    <div className="relative w-20 h-20 flex items-center justify-center">
                      <svg className="w-full h-full transform -rotate-90">
                        <circle cx="40" cy="40" r="34" stroke="rgba(255,255,255,0.06)" strokeWidth="6" fill="transparent" />
                        <circle
                          cx="40"
                          cy="40"
                          r="34"
                          stroke={activeGame.report.accuracyWhite >= 80 ? '#10b981' : activeGame.report.accuracyWhite >= 60 ? '#f59e0b' : '#ef4444'}
                          strokeWidth="6"
                          fill="transparent"
                          strokeDasharray={2 * Math.PI * 34}
                          strokeDashoffset={2 * Math.PI * 34 * (1 - activeGame.report.accuracyWhite / 100)}
                          strokeLinecap="round"
                        />
                      </svg>
                      <span className="absolute text-base font-extrabold text-slate-100">{activeGame.report.accuracyWhite}%</span>
                    </div>
                    <div className="text-[9px] text-textMuted flex space-x-1.5 font-mono">
                      <span className="text-cyan-400">✨{activeGame.report.whiteBrilliants}</span>
                      <span className="text-red-400">⚠️{activeGame.report.whiteBlunders}</span>
                      <span className="text-orange-400">❌{activeGame.report.whiteMistakes}</span>
                    </div>
                  </div>

                  {/* Black Accuracy */}
                  <div className="flex flex-col items-center space-y-2">
                    <span className="text-[10px] text-textMuted font-bold uppercase tracking-wider">Black Accuracy</span>
                    <div className="relative w-20 h-20 flex items-center justify-center">
                      <svg className="w-full h-full transform -rotate-90">
                        <circle cx="40" cy="40" r="34" stroke="rgba(255,255,255,0.06)" strokeWidth="6" fill="transparent" />
                        <circle
                          cx="40"
                          cy="40"
                          r="34"
                          stroke={activeGame.report.accuracyBlack >= 80 ? '#10b981' : activeGame.report.accuracyBlack >= 60 ? '#f59e0b' : '#ef4444'}
                          strokeWidth="6"
                          fill="transparent"
                          strokeDasharray={2 * Math.PI * 34}
                          strokeDashoffset={2 * Math.PI * 34 * (1 - activeGame.report.accuracyBlack / 100)}
                          strokeLinecap="round"
                        />
                      </svg>
                      <span className="absolute text-base font-extrabold text-slate-100">{activeGame.report.accuracyBlack}%</span>
                    </div>
                    <div className="text-[9px] text-textMuted flex space-x-1.5 font-mono">
                      <span className="text-cyan-400">✨{activeGame.report.blackBrilliants}</span>
                      <span className="text-red-400">⚠️{activeGame.report.blackBlunders}</span>
                      <span className="text-orange-400">❌{activeGame.report.blackMistakes}</span>
                    </div>
                  </div>
                </div>
              )}

              {/* AI Coaching Advice */}
              {activeGame.report && (
                <div className="bg-gradient-to-r from-primary/10 to-indigo-500/10 border border-primary/20 rounded-2xl p-4 shadow-md space-y-2">
                  <h4 className="text-xs font-bold text-slate-200 flex items-center space-x-1">
                    <span>💡</span>
                    <span>Positional AI Coach Feedback</span>
                  </h4>
                  <p className="text-xs text-slate-300 leading-relaxed italic">
                    "{activeGame.report.trainingAdvice}"
                  </p>
                  
                  {activeGame.report.endgame && (
                    <div className="pt-2 border-t border-borderColor/30 text-[10px] text-textMuted">
                      <span className="font-bold text-slate-300">Theoretical Endings:</span> {activeGame.report.endgame}
                    </div>
                  )}
                </div>
              )}

              {/* Interactive Timeline Moves List */}
              <div className="bg-panelBg/80 border border-borderColor/50 rounded-2xl shadow-xl flex flex-col h-[280px]">
                <div className="p-3 border-b border-borderColor/40 flex items-center justify-between">
                  <span className="text-[10px] font-bold tracking-wider text-textMuted uppercase">Play Timeline moves list</span>
                  <span className="text-[9px] text-textMuted">Click ply to inspect</span>
                </div>

                <div className="flex-1 overflow-y-auto p-3 grid grid-cols-3 gap-2 scrollbar-thin align-content-start">
                  {activeGame.moves.map((move, idx) => {
                    const isSelected = currentIndex === idx + 1;
                    const pillClass = getClassificationColor(move.classification);

                    return (
                      <button
                        key={idx}
                        onClick={() => setCurrentIndex(idx + 1)}
                        className={`p-2 rounded-xl text-left border transition-all text-xs flex flex-col justify-between ${
                          isSelected
                            ? 'bg-primary/20 border-primary text-slate-100 ring-2 ring-primary/40'
                            : 'bg-accentBg/30 border-borderColor/30 hover:bg-accentBg/60 text-slate-300'
                        }`}
                      >
                        <div className="flex justify-between items-center w-full">
                          <span className="font-mono text-[10px] text-textMuted">{idx + 1}.</span>
                          <span className="font-bold">{move.san}</span>
                        </div>
                        <span className={`text-[8px] px-1 py-0.5 mt-1 rounded text-center w-full truncate font-semibold ${pillClass}`}>
                          {move.classification}
                        </span>
                      </button>
                    );
                  })}
                </div>
              </div>

              {/* Selected Move Explanation Panel */}
              {currentIndex > 0 && activeGame.moves[currentIndex - 1] && (
                <div className="bg-panelBg border border-borderColor rounded-2xl p-4 shadow-xl space-y-3">
                  <div className="flex items-center justify-between border-b border-borderColor/40 pb-2">
                    <h4 className="text-xs font-bold text-slate-100">
                      Analysis for Move {currentIndex}: <span className="text-indigo-400 font-mono">{activeGame.moves[currentIndex - 1].san}</span>
                    </h4>
                    <span className={`text-[9px] px-2 py-0.5 rounded-full font-bold uppercase ${getClassificationColor(activeGame.moves[currentIndex - 1].classification)}`}>
                      {activeGame.moves[currentIndex - 1].classification}
                    </span>
                  </div>

                  <div className="space-y-2 text-xs">
                    <p className="text-slate-300 leading-relaxed">
                      {activeGame.moves[currentIndex - 1].explanation || 'Engine maintains positional status-quo.'}
                    </p>

                    {/* Score / Delta info */}
                    <div className="grid grid-cols-3 gap-2 pt-2 text-[10px] font-mono text-textMuted bg-accentBg/30 p-2 rounded-xl border border-borderColor/30">
                      <div>
                        <span className="block text-[8px] uppercase tracking-wider text-slate-500">Eval Score</span>
                        <span className="text-slate-200">{(activeGame.moves[currentIndex - 1].score / 100.0).toFixed(2)}</span>
                      </div>
                      <div>
                        <span className="block text-[8px] uppercase tracking-wider text-slate-500">Eval Loss</span>
                        <span className="text-slate-200">{(activeGame.moves[currentIndex - 1].delta / 100.0).toFixed(2)}</span>
                      </div>
                      <div>
                        <span className="block text-[8px] uppercase tracking-wider text-slate-500">Best Option</span>
                        <span className="text-indigo-400 font-semibold">{activeGame.moves[currentIndex - 1].bestMove || 'none'}</span>
                      </div>
                    </div>

                    {/* Tactical / Positional Warnings */}
                    {(activeGame.moves[currentIndex - 1].tacticalMistake || activeGame.moves[currentIndex - 1].positionalMistake) && (
                      <div className="mt-3 p-3 bg-yellow-500/10 border border-yellow-500/20 text-yellow-400 rounded-xl space-y-1">
                        {activeGame.moves[currentIndex - 1].tacticalMistake && (
                          <div className="flex items-center space-x-1.5 font-semibold text-[11px]">
                            <AlertTriangle size={12} />
                            <span>Tactical: {activeGame.moves[currentIndex - 1].tacticalMistake}</span>
                          </div>
                        )}
                        {activeGame.moves[currentIndex - 1].positionalMistake && (
                          <div className="flex items-center space-x-1.5 font-semibold text-[11px]">
                            <AlertTriangle size={12} />
                            <span>Positional: {activeGame.moves[currentIndex - 1].positionalMistake}</span>
                          </div>
                        )}
                      </div>
                    )}
                  </div>
                </div>
              )}

            </div>

          </div>
        ) : (
          <div className="flex-1 flex flex-col items-center justify-center text-center py-20 max-w-md mx-auto space-y-6">
            <div className="w-16 h-16 rounded-full bg-primary/10 border border-primary/20 flex items-center justify-center text-primary shadow-xl shadow-indigo-600/5">
              <Upload size={28} />
            </div>
            <div className="space-y-2">
              <h3 className="text-base font-bold text-slate-100">No Chess Game Loaded</h3>
              <p className="text-xs text-textMuted leading-relaxed">
                Paste a PGN string in the left panel or click the demo button to load a master-level game and see the automated AI analysis in action.
              </p>
            </div>
            <button
              onClick={() => handleImport(samplePgn)}
              className="px-6 py-2.5 bg-primary hover:bg-indigo-500 text-white rounded-xl text-xs font-semibold shadow-lg shadow-indigo-600/15 cursor-pointer"
            >
              Load Demo Master Game
            </button>
          </div>
        )}
      </main>
    </div>
  );
};
export default GameIntelligence;
