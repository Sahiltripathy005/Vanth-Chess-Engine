export interface EvaluationBreakdown {
    material: number;
    pst: number;
    pawn_structure: number;
    mobility: number;
    piece_activity: number;
    king_safety: number;
    space: number;
    threats: number;
    endgame: number;
    tempo: number;
    total: number;
}
export interface AlternativeMove {
    move: string;
    score: number;
    pv: string[];
}
export interface MoveExplanation {
    move: string;
    reasons: string[];
}
export interface TacticalThemes {
    fork: boolean;
    pin: boolean;
    skewer: boolean;
    discovered_attack: boolean;
    double_attack: boolean;
    deflection: boolean;
    decoy: boolean;
    x_ray: boolean;
    back_rank_weakness: boolean;
    remove_defender: boolean;
    zwischenzug: boolean;
    descriptions: string[];
}
export interface PositionClassifications {
    opening: boolean;
    middlegame: boolean;
    endgame: boolean;
    closed_position: boolean;
    open_position: boolean;
    semi_open: boolean;
    tactical: boolean;
    strategic: boolean;
    king_attack: boolean;
    pawn_race: boolean;
    labels: string[];
}
export interface SearchStats {
    nodes: number;
    nps: number;
    time_ms: number;
    cutoffs: number;
    tt_hits: number;
}
export interface FullAnalysisReport {
    fen: string;
    best_move: string;
    evaluation: number;
    breakdown: EvaluationBreakdown;
    alternatives: AlternativeMove[];
    explanation: MoveExplanation;
    tactical_themes: TacticalThemes;
    classifications: PositionClassifications;
    strengths: string[];
    weaknesses: string[];
    search_stats: SearchStats;
}
export interface BlunderAnalysis {
    move_played: string;
    score_drop: number;
    is_blunder: boolean;
    explanation: string;
}
export interface EngineSettings {
    depth: number;
    hash: number;
    threads: number;
    multiPV: number;
    searchTime: number;
    ownBook: boolean;
    ponder: boolean;
}
