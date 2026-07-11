import { create } from 'zustand';
import { FullAnalysisReport, EngineSettings } from '@vanth/shared';
import { START_FEN, parseFen } from '../utils/chess';

export interface MoveTreeNode {
  id: string;
  move: string;
  fen: string;
  parentId: string | null;
  children: string[];
}

export interface LiveAnalysisInfo {
  depth: number;
  seldepth: number;
  scoreType: 'cp' | 'mate';
  score: number;
  nodes: number;
  nps: number;
  timeMs: number;
  pv: string[];
}

interface GameState {
  currentFen: string;
  moveHistory: string[];
  historyIndex: number;
  fenHistory: string[];
  flipped: boolean;
  selectedSquare: string | null;
  legalMoves: string[];
  analysisReport: FullAnalysisReport | null;
  isLoadingAnalysis: boolean;
  isLiveSearching: boolean;
  evalScore: number;
  availableLegalMoves: string[];

  // Move tree properties
  moveTree: Record<string, MoveTreeNode>;
  rootNodeId: string;
  currentNodeId: string;

  // Live streaming search properties
  liveAnalysisInfo: LiveAnalysisInfo | null;
  activeEventSource: EventSource | null;

  savedPositions: any[];
  analysisHistoryList: any[];
  gameHistoryList: any[];

  settings: EngineSettings;
  theme: 'dark' | 'light';
  boardTheme: 'emerald' | 'wood' | 'slate' | 'classic';
  pieceTheme: 'neo' | 'classic';

  setFen: (fen: string) => void;
  resetGame: () => void;
  undoMove: () => void;
  redoMove: () => void;
  jumpToHistory: (index: number) => void;
  jumpToNode: (nodeId: string) => void;
  syncFromCurrentNode: () => void;
  flipBoard: () => void;
  selectSquare: (square: string | null) => void;
  setLegalMoves: (moves: string[]) => void;
  playMove: (from: string, to: string, promotion?: string) => Promise<boolean>;
  
  fetchAvailableLegalMoves: () => Promise<void>;
  fetchSettings: () => Promise<void>;
  updateSettings: (newSettings: Partial<EngineSettings & { theme: string; boardTheme: string; pieceTheme: string }>) => Promise<void>;
  runAnalysis: () => Promise<void>;
  cancelAnalysis: () => void;
  fetchSavedPositions: () => Promise<void>;
  saveCurrentPosition: (name: string, notes?: string) => Promise<void>;
  deleteSavedPosition: (id: string) => Promise<void>;
  fetchAnalysisHistoryList: () => Promise<void>;
  fetchGameHistoryList: () => Promise<void>;
  saveGame: (pgn: string, result: string, white: string, black: string) => Promise<void>;
}

const initialRootNode: MoveTreeNode = {
  id: 'root',
  move: '',
  fen: START_FEN,
  parentId: null,
  children: []
};

export const useGameStore = create<GameState>()((set, get) => ({
  currentFen: START_FEN,
  moveHistory: [],
  historyIndex: 0,
  fenHistory: [START_FEN],
  flipped: false,
  selectedSquare: null,
  legalMoves: [],
  analysisReport: null,
  isLoadingAnalysis: false,
  isLiveSearching: false,
  evalScore: 0,
  availableLegalMoves: [],

  moveTree: { root: initialRootNode },
  rootNodeId: 'root',
  currentNodeId: 'root',

  liveAnalysisInfo: null,
  activeEventSource: null,

  savedPositions: [],
  analysisHistoryList: [],
  gameHistoryList: [],

  settings: {
    depth: 4,
    hash: 16,
    threads: 1,
    multiPV: 3,
    searchTime: 1000,
    ownBook: true,
    ponder: false,
  },
  theme: 'dark',
  boardTheme: 'emerald',
  pieceTheme: 'neo',

  setFen: (fen: string) => {
    const rootNode: MoveTreeNode = {
      id: 'root',
      move: '',
      fen: fen,
      parentId: null,
      children: []
    };
    set({
      moveTree: { root: rootNode },
      rootNodeId: 'root',
      currentNodeId: 'root',
      currentFen: fen,
      moveHistory: [],
      fenHistory: [fen],
      historyIndex: 0,
      selectedSquare: null,
      legalMoves: [],
      analysisReport: null,
      liveAnalysisInfo: null
    });
    get().fetchAvailableLegalMoves();
  },

  resetGame: () => {
    const rootNode: MoveTreeNode = {
      id: 'root',
      move: '',
      fen: START_FEN,
      parentId: null,
      children: []
    };
    set({
      moveTree: { root: rootNode },
      rootNodeId: 'root',
      currentNodeId: 'root',
      currentFen: START_FEN,
      moveHistory: [],
      fenHistory: [START_FEN],
      historyIndex: 0,
      selectedSquare: null,
      legalMoves: [],
      analysisReport: null,
      evalScore: 0,
      liveAnalysisInfo: null
    });
    get().fetchAvailableLegalMoves();
  },

  undoMove: () => {
    const { moveTree, currentNodeId } = get();
    const node = moveTree[currentNodeId];
    if (node && node.parentId) {
      set({ currentNodeId: node.parentId });
      get().syncFromCurrentNode();
    }
  },

  redoMove: () => {
    const { moveTree, currentNodeId } = get();
    const node = moveTree[currentNodeId];
    if (node && node.children.length > 0) {
      set({ currentNodeId: node.children[0] });
      get().syncFromCurrentNode();
    }
  },

  jumpToHistory: (index: number) => {
    const { moveTree, currentNodeId } = get();
    const pathIds: string[] = [];
    let curr = moveTree[currentNodeId];
    while (curr) {
      pathIds.push(curr.id);
      curr = curr.parentId ? moveTree[curr.parentId] : null as any;
    }
    pathIds.reverse();
    if (index >= 0 && index < pathIds.length) {
      set({ currentNodeId: pathIds[index] });
      get().syncFromCurrentNode();
    }
  },

  jumpToNode: (nodeId: string) => {
    const { moveTree } = get();
    if (moveTree[nodeId]) {
      set({ currentNodeId: nodeId });
      get().syncFromCurrentNode();
    }
  },

  syncFromCurrentNode: () => {
    const { moveTree, currentNodeId } = get();
    const pathNodes: MoveTreeNode[] = [];
    let curr = moveTree[currentNodeId];
    while (curr) {
      pathNodes.push(curr);
      curr = curr.parentId ? moveTree[curr.parentId] : null as any;
    }
    pathNodes.reverse();

    const moveHistory = pathNodes.slice(1).map(n => n.move);
    const fenHistory = pathNodes.map(n => n.fen);

    set({
      currentFen: moveTree[currentNodeId].fen,
      moveHistory,
      fenHistory,
      historyIndex: moveHistory.length,
      selectedSquare: null,
      legalMoves: [],
      analysisReport: null
    });

    get().fetchAvailableLegalMoves();
  },

  flipBoard: () => set((state) => ({ flipped: !state.flipped })),

  selectSquare: (square: string | null) => {
    if (!square) {
      set({ selectedSquare: null, legalMoves: [] });
      return;
    }

    const { currentFen, availableLegalMoves } = get();
    const board = parseFen(currentFen);
    
    const file = square.charCodeAt(0) - 'a'.charCodeAt(0);
    const rank = 8 - parseInt(square[1], 10);
    const piece = board[rank][file];
    const activeColor = currentFen.split(' ')[1];

    if (piece && piece.color === activeColor) {
      const pieceMoves = availableLegalMoves
        .filter((m: string) => m.startsWith(square))
        .map((m: string) => m.substring(2, 4));
      
      set({ selectedSquare: square, legalMoves: pieceMoves });
    } else {
      const { selectedSquare } = get();
      if (selectedSquare) {
        const isLegal = availableLegalMoves.some((m: string) => m.startsWith(selectedSquare) && m.endsWith(square));
        if (isLegal) {
          get().playMove(selectedSquare, square);
        } else {
          set({ selectedSquare: null, legalMoves: [] });
        }
      }
    }
  },

  setLegalMoves: (moves: string[]) => set({ legalMoves: moves }),

  playMove: async (from: string, to: string, promotion = '') => {
    const { currentFen, moveTree, currentNodeId, availableLegalMoves } = get();
    
    const playedMove = `${from}${to}${promotion}`;
    const match = availableLegalMoves.find((m: string) => m === playedMove || m === `${from}${to}`);
    
    if (!match) return false;

    // Check if one of the children of currentNodeId already matches this move
    const currentNode = moveTree[currentNodeId];
    const existingChildId = currentNode.children.find(childId => moveTree[childId].move === match);
    
    if (existingChildId) {
      set({ currentNodeId: existingChildId });
      get().syncFromCurrentNode();
      return true;
    }

    try {
      const res = await fetch('/api/move', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ fen: currentFen, move: match })
      });
      if (res.ok) {
        const data = await res.json();
        const nextFen = data.fen;

        const newNodeId = `node-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
        const newNode: MoveTreeNode = {
          id: newNodeId,
          move: match,
          fen: nextFen,
          parentId: currentNodeId,
          children: []
        };

        const updatedMoveTree = {
          ...moveTree,
          [newNodeId]: newNode,
          [currentNodeId]: {
            ...currentNode,
            children: [...currentNode.children, newNodeId]
          }
        };

        set({
          moveTree: updatedMoveTree,
          currentNodeId: newNodeId
        });

        get().syncFromCurrentNode();
        return true;
      }
    } catch (e) {
      console.error('Failed to play move:', e);
    }
    return false;
  },

  fetchAvailableLegalMoves: async () => {
    const { currentFen } = get();
    try {
      const res = await fetch('/api/legalmoves', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ fen: currentFen })
      });
      const data = await res.json();
      set({ availableLegalMoves: data.moves || [] });
    } catch (e) {
      console.error('Failed to fetch legal moves:', e);
    }
  },

  fetchSettings: async () => {
    try {
      const res = await fetch('/api/settings');
      if (res.ok) {
        const data = await res.json();
        set({
          settings: {
            depth: data.defaultDepth,
            hash: data.hashSize,
            threads: data.threads,
            multiPV: data.multiPV || 3,
            searchTime: data.searchTime,
            ownBook: data.ownBook,
            ponder: data.ponder,
          },
          theme: (data.theme as any) || 'dark',
          boardTheme: (data.boardTheme as any) || 'emerald',
          pieceTheme: (data.pieceTheme as any) || 'neo'
        });
      }
    } catch (e) {
      console.error(e);
    }
  },

  updateSettings: async (newSettings: Partial<EngineSettings & { theme: string; boardTheme: string; pieceTheme: string }>) => {
    try {
      const current = get();
      const payload = {
        theme: newSettings.theme ?? current.theme,
        boardTheme: newSettings.boardTheme ?? current.boardTheme,
        pieceTheme: newSettings.pieceTheme ?? current.pieceTheme,
        defaultDepth: newSettings.depth ?? current.settings.depth,
        threads: newSettings.threads ?? current.settings.threads,
        hashSize: newSettings.hash ?? current.settings.hash,
        searchTime: newSettings.searchTime ?? current.settings.searchTime,
        ownBook: newSettings.ownBook ?? current.settings.ownBook,
        ponder: newSettings.ponder ?? current.settings.ponder,
      };

      const res = await fetch('/api/settings', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      if (res.ok) {
        const data = await res.json();
        set({
          settings: {
            depth: data.defaultDepth,
            hash: data.hashSize,
            threads: data.threads,
            multiPV: data.multiPV || 3,
            searchTime: data.searchTime,
            ownBook: data.ownBook,
            ponder: data.ponder,
          },
          theme: data.theme,
          boardTheme: data.boardTheme,
          pieceTheme: data.pieceTheme
        });
      }
    } catch (e) {
      console.error(e);
    }
  },

  runAnalysis: async () => {
    const { currentFen, settings, activeEventSource } = get();
    
    // Close existing stream if any
    if (activeEventSource) {
      activeEventSource.close();
    }

    set({
      isLoadingAnalysis: true,
      isLiveSearching: true,
      analysisReport: null,
      liveAnalysisInfo: null
    });

    try {
      const url = `/api/analyze/stream?fen=${encodeURIComponent(currentFen)}&depth=${settings.depth}`;
      const es = new EventSource(url);
      set({ activeEventSource: es });

      es.onmessage = (event) => {
        const msg = JSON.parse(event.data);
        if (msg.type === 'info') {
          set({
            liveAnalysisInfo: msg.data,
            evalScore: msg.data.score
          });
        } else if (msg.type === 'report') {
          set({
            analysisReport: msg.data,
            evalScore: msg.data.evaluation || 0
          });
          get().fetchAnalysisHistoryList();
        } else if (msg.type === 'done') {
          es.close();
          set({
            isLoadingAnalysis: false,
            isLiveSearching: false,
            activeEventSource: null
          });
        }
      };

      es.onerror = (err) => {
        console.error('SSE connection error:', err);
        es.close();
        set({
          isLoadingAnalysis: false,
          isLiveSearching: false,
          activeEventSource: null
        });
      };
    } catch (e) {
      console.error('Failed to run live analysis stream:', e);
      set({
        isLoadingAnalysis: false,
        isLiveSearching: false,
        activeEventSource: null
      });
    }
  },

  cancelAnalysis: () => {
    const { activeEventSource } = get();
    if (activeEventSource) {
      activeEventSource.close();
      console.log('User canceled live analysis search.');
    }
    set({
      isLoadingAnalysis: false,
      isLiveSearching: false,
      activeEventSource: null
    });
  },

  fetchSavedPositions: async () => {
    try {
      const res = await fetch('/api/saved-positions');
      if (res.ok) {
        const data = await res.json();
        set({ savedPositions: data });
      }
    } catch (e) {
      console.error(e);
    }
  },

  saveCurrentPosition: async (name: string, notes?: string) => {
    const { currentFen } = get();
    try {
      const res = await fetch('/api/saved-positions', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ fen: currentFen, name, notes })
      });
      if (res.ok) {
        get().fetchSavedPositions();
      }
    } catch (e) {
      console.error(e);
    }
  },

  deleteSavedPosition: async (id: string) => {
    try {
      const res = await fetch(`/api/saved-positions/${id}`, {
        method: 'DELETE'
      });
      if (res.ok) {
        get().fetchSavedPositions();
      }
    } catch (e) {
      console.error(e);
    }
  },

  fetchAnalysisHistoryList: async () => {
    try {
      const res = await fetch('/api/history');
      if (res.ok) {
        const data = await res.json();
        set({ analysisHistoryList: data });
      }
    } catch (e) {
      console.error(e);
    }
  },

  fetchGameHistoryList: async () => {
    try {
      const res = await fetch('/api/games');
      if (res.ok) {
        const data = await res.json();
        set({ gameHistoryList: data });
      }
    } catch (e) {
      console.error(e);
    }
  },

  saveGame: async (pgn: string, result: string, white: string, black: string) => {
    try {
      const res = await fetch('/api/games', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pgn, result, whitePlayer: white, blackPlayer: black })
      });
      if (res.ok) {
        get().fetchGameHistoryList();
      }
    } catch (e) {
      console.error(e);
    }
  }
}));
