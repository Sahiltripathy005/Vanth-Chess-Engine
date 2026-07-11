import React from 'react';
import { useGameStore, MoveTreeNode } from '../store/gameStore';
import { TrendingUp, TrendingDown, Minus } from 'lucide-react';

export const EvaluationGraph: React.FC = () => {
  const { moveTree, currentNodeId, jumpToNode } = useGameStore();

  // Reconstruct path nodes from root to current node
  const pathNodes: MoveTreeNode[] = [];
  let curr = moveTree[currentNodeId];
  while (curr) {
    pathNodes.push(curr);
    curr = curr.parentId ? moveTree[curr.parentId] : null as any;
  }
  pathNodes.reverse();

  if (pathNodes.length <= 1) {
    return (
      <div className="h-20 bg-accentBg/20 border border-borderColor/40 rounded-xl flex items-center justify-center text-xs text-textMuted select-none">
        <Minus size={14} className="mr-1" />
        <span>Evaluation history will graph here as you play and analyze.</span>
      </div>
    );
  }

  // Cap score between -800 (Black winning) and +800 (White winning)
  const getCoords = (idx: number, score: number, width: number, height: number) => {
    const totalPoints = pathNodes.length;
    const x = totalPoints > 1 ? (idx / (totalPoints - 1)) * (width - 24) + 12 : width / 2;
    
    // Scale score: +800 is near top, -800 is near bottom, 0 is middle
    const clamped = Math.max(-800, Math.min(800, score));
    const normalized = (clamped + 800) / 1600; // 0 to 1
    const y = height - (normalized * (height - 20) + 10);
    return { x, y };
  };

  const width = 500;
  const height = 80;

  // Build path points
  const points = pathNodes.map((node, idx) => {
    // If node doesn't have an analysis score, we search if it is in the analysisHistoryList or default to 0
    const nodeScore = (node as any).score !== undefined ? (node as any).score : 0;
    return {
      nodeId: node.id,
      move: node.move || 'Start',
      score: nodeScore,
      ...getCoords(idx, nodeScore, width, height)
    };
  });

  // SVG Path description
  let pathD = '';
  if (points.length > 0) {
    pathD = `M ${points[0].x} ${points[0].y}`;
    for (let i = 1; i < points.length; i++) {
      pathD += ` L ${points[i].x} ${points[i].y}`;
    }
  }

  // Closed path description for gradient fill
  const fillD = points.length > 0 
    ? `${pathD} L ${points[points.length - 1].x} ${height} L ${points[0].x} ${height} Z` 
    : '';

  // Determine trend indicators
  const latestScore = points[points.length - 1].score / 100.0;
  const isWhiteWinning = latestScore > 0.5;
  const isBlackWinning = latestScore < -0.5;

  return (
    <div className="bg-cardBg border border-borderColor rounded-xl p-4 space-y-3 shadow-md select-none">
      <div className="flex items-center justify-between">
        <span className="text-[10px] text-textMuted uppercase font-bold tracking-wider">Evaluation Timeline</span>
        <div className="flex items-center space-x-1 text-xs">
          {isWhiteWinning ? (
            <div className="flex items-center text-secondary font-semibold">
              <TrendingUp size={14} className="mr-1" />
              <span>White Advantage (+{latestScore.toFixed(2)})</span>
            </div>
          ) : isBlackWinning ? (
            <div className="flex items-center text-danger font-semibold">
              <TrendingDown size={14} className="mr-1" />
              <span>Black Advantage ({latestScore.toFixed(2)})</span>
            </div>
          ) : (
            <div className="flex items-center text-textMuted">
              <Minus size={14} className="mr-1" />
              <span>Even Position</span>
            </div>
          )}
        </div>
      </div>

      <div className="relative w-full h-[80px]">
        <svg 
          viewBox={`0 0 ${width} ${height}`} 
          className="w-full h-full overflow-visible"
          preserveAspectRatio="none"
        >
          {/* Middle Zero Balance Line */}
          <line
            x1={0}
            y1={height / 2}
            x2={width}
            y2={height / 2}
            stroke="var(--border-color)"
            strokeDasharray="4 4"
            strokeOpacity="0.4"
          />

          {/* Gradient Fill under graph */}
          {fillD && (
            <path
              d={fillD}
              fill="url(#graph-grad)"
              opacity="0.15"
            />
          )}

          {/* Main Line */}
          {pathD && (
            <path
              d={pathD}
              fill="none"
              stroke="var(--color-primary)"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          )}

          {/* Interactive dots */}
          {points.map((pt) => {
            const isActive = pt.nodeId === currentNodeId;
            return (
              <g key={pt.nodeId} className="group cursor-pointer" onClick={() => jumpToNode(pt.nodeId)}>
                <circle
                  cx={pt.x}
                  cy={pt.y}
                  r={isActive ? 5 : 3.5}
                  className={`${
                    isActive 
                      ? 'fill-primary stroke-white stroke-2' 
                      : 'fill-[#171717] stroke-primary stroke-1.5 hover:fill-primary hover:r-5 transition-all'
                  }`}
                />
                
                {/* Tooltip on hover */}
                <title>
                  {pt.move === 'Start' ? 'Initial Position' : `Move: ${pt.move}`}
                  {`\nEval: ${(pt.score / 100.0).toFixed(2)}`}
                </title>
              </g>
            );
          })}

          <defs>
            <linearGradient id="graph-grad" x1="0" y1="0" x2="0" y2="1">
              <stop offset="0%" stopColor="var(--color-primary)" />
              <stop offset="100%" stopColor="var(--color-primary)" stopOpacity="0" />
            </linearGradient>
          </defs>
        </svg>
      </div>
    </div>
  );
};
