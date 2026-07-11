import React from 'react';
import { useGameStore } from '../store/gameStore';
import { ChevronLeft, ChevronRight, RotateCcw } from 'lucide-react';

export const MoveList: React.FC = () => {
  const {
    moveTree,
    rootNodeId,
    currentNodeId,
    jumpToNode,
    undoMove,
    redoMove,
    resetGame,
    moveHistory
  } = useGameStore();

  // Recursive walker to build the move tree markup
  const renderMoveNode = (nodeId: string, moveNum: number, isWhite: boolean, isInsideVar = false): React.ReactNode[] => {
    const node = moveTree[nodeId];
    if (!node || node.children.length === 0) return [];

    const nodesToRender: React.ReactNode[] = [];
    const mainChildId = node.children[0];
    const mainChild = moveTree[mainChildId];

    const isMainSelected = currentNodeId === mainChildId;

    // Render move number label
    if (isWhite) {
      nodesToRender.push(
        <span key={`num-${mainChildId}`} className="text-textMuted font-mono mr-1 select-none text-[11px]">
          {moveNum}.
        </span>
      );
    } else if (isInsideVar) {
      nodesToRender.push(
        <span key={`num-var-${mainChildId}`} className="text-textMuted font-mono mr-1 select-none text-[11px]">
          {moveNum}...
        </span>
      );
    }

    // Main move node element
    nodesToRender.push(
      <span
        key={mainChildId}
        onClick={() => jumpToNode(mainChildId)}
        className={`px-1.5 py-0.5 rounded font-mono text-xs font-semibold cursor-pointer transition-all mr-1.5 ${
          isMainSelected
            ? 'bg-primary text-white shadow-md'
            : 'text-slate-200 hover:bg-accentBg/60 hover:text-white'
        }`}
      >
        {mainChild.move}
      </span>
    );

    // Variations (Alternative siblings)
    const variations = node.children.slice(1);
    for (const varId of variations) {
      const varNode = moveTree[varId];
      const isVarSelected = currentNodeId === varId;

      nodesToRender.push(
        <span key={`var-group-${varId}`} className="text-slate-400 bg-accentBg/20 border border-borderColor/20 rounded-md px-1.5 py-0.5 mx-1 inline-flex items-center flex-wrap select-text">
          <span className="text-slate-500 font-bold mr-1 select-none font-mono text-[10px]">(</span>
          
          <span
            onClick={() => jumpToNode(varId)}
            className={`px-1 rounded font-mono text-xs font-semibold cursor-pointer transition-colors ${
              isVarSelected
                ? 'bg-primary text-white'
                : 'text-indigo-300 hover:bg-accentBg/70'
            }`}
          >
            {moveNum}{isWhite ? '.' : '...'} {varNode.move}
          </span>
          
          {/* Nested recursion */}
          {renderMoveNode(varId, isWhite ? moveNum : moveNum + 1, !isWhite, true)}
          
          <span className="text-slate-500 font-bold ml-1 select-none font-mono text-[10px]">)</span>
        </span>
      );
    }

    // Continue down the main line path
    nodesToRender.push(
      ...renderMoveNode(mainChildId, isWhite ? moveNum : moveNum + 1, !isWhite, false)
    );

    return nodesToRender;
  };

  const hasMoves = moveTree[rootNodeId]?.children.length > 0;

  return (
    <div className="flex flex-col h-full bg-panelBg border-l border-borderColor overflow-hidden select-none">
      {/* Top Header controls */}
      <div className="flex items-center justify-between p-4 border-b border-borderColor bg-cardBg">
        <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Move Tree</span>
        <button
          onClick={resetGame}
          className="p-1 text-textMuted hover:text-white rounded hover:bg-slate-700 transition-colors cursor-pointer"
          title="Reset Board"
        >
          <RotateCcw size={12} />
        </button>
      </div>

      {/* History scroll list */}
      <div className="flex-1 overflow-y-auto p-4 leading-loose select-text scrollbar-thin">
        {!hasMoves ? (
          <div className="text-center text-textMuted py-8 italic text-[11px]">
            No moves recorded yet. Play moves on the board to form lines.
          </div>
        ) : (
          <div className="flex flex-wrap items-center gap-y-2.5">
            {renderMoveNode(rootNodeId, 1, true, false)}
          </div>
        )}
      </div>

      {/* Navigation Buttons */}
      <div className="flex items-center justify-center p-3 border-t border-borderColor/60 bg-cardBg/30 space-x-4">
        <button
          onClick={undoMove}
          disabled={currentNodeId === rootNodeId}
          className="p-1.5 bg-accentBg border border-borderColor hover:bg-slate-700 disabled:opacity-40 rounded transition-all cursor-pointer"
          title="Prev Move"
        >
          <ChevronLeft size={16} />
        </button>
        <span className="text-xs font-mono text-textMuted">
          Move {moveHistory.length}
        </span>
        <button
          onClick={redoMove}
          disabled={!moveTree[currentNodeId] || moveTree[currentNodeId].children.length === 0}
          className="p-1.5 bg-accentBg border border-borderColor hover:bg-slate-700 disabled:opacity-40 rounded transition-all cursor-pointer"
          title="Next Move"
        >
          <ChevronRight size={16} />
        </button>
      </div>
    </div>
  );
};
