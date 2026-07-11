import React from 'react';

interface ChessPieceSvgProps {
  type: 'p' | 'n' | 'b' | 'r' | 'q' | 'k';
  color: 'w' | 'b';
  className?: string;
}

export const ChessPieceSvg: React.FC<ChessPieceSvgProps> = ({ type, color, className = "w-full h-full" }: ChessPieceSvgProps) => {
  const isWhite = color === 'w';

  switch (type) {
    case 'p':
      return (
        <svg viewBox="0 0 45 45" className={className}>
          <path
            d="M 22,9 C 19.79,9 18,10.79 18,13 C 18,13.89 18.29,14.71 18.78,15.38 C 16.83,16.5 15.5,18.59 15.5,21 C 15.5,23.03 16.44,24.84 17.91,26.03 C 14.91,27.09 10.5,31.58 10.5,36 L 34.5,36 C 34.5,31.58 30.09,27.09 27.09,26.03 C 28.56,24.84 29.5,23.03 29.5,21 C 29.5,18.59 28.17,16.5 26.22,15.38 C 26.71,14.71 27,13.89 27,13 C 27,10.79 25.21,9 23,9 C 22.66,9 22.33,9.05 22,9 z"
            fill={isWhite ? "#ffffff" : "#262626"}
            stroke={isWhite ? "#262626" : "#ffffff"}
            strokeWidth="1.5"
            strokeLinejoin="round"
          />
        </svg>
      );
    case 'r':
      return (
        <svg viewBox="0 0 45 45" className={className}>
          <g
            fill={isWhite ? "#ffffff" : "#262626"}
            stroke={isWhite ? "#262626" : "#ffffff"}
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <path d="M 9,39 L 36,39 L 36,36 L 9,36 L 9,39 z" />
            <path d="M 12,36 L 12,32 L 33,32 L 33,36 L 12,36 z" />
            <path d="M 12,32 L 14,14 L 31,14 L 33,32 L 12,32 z" />
            <path d="M 14,14 L 14,9 L 17,9 L 17,11 L 20,11 L 20,9 L 25,9 L 25,11 L 28,11 L 28,9 L 31,9 L 31,14 L 14,14 z" />
            <path d="M 11,14 L 34,14" fill="none" stroke={isWhite ? "#262626" : "#ffffff"} />
            <path d="M 12,32 L 33,32" fill="none" stroke={isWhite ? "#262626" : "#ffffff"} />
          </g>
        </svg>
      );
    case 'n':
      return (
        <svg viewBox="0 0 45 45" className={className}>
          <g
            fill={isWhite ? "#ffffff" : "#262626"}
            stroke={isWhite ? "#262626" : "#ffffff"}
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <path d="M 22,10 C 22,10 19,11 16,15 C 13,19 13,23 13,23 C 13,23 14,20 18,20 C 18,20 17,21 15,24 C 13,27 13,31 13,31 C 13,31 16,31 18,28 C 20,25 21,24 23,24 C 25,24 27,25 28,27 C 29,29 31,31 31,31 C 31,31 32,27 30,24 C 28,21 27,20 27,20 C 31,20 32,23 32,23 C 32,23 32,19 29,15 C 26,11 23,10 22,10 z" />
            <path d="M 9.5,38.5 L 35.5,38.5" fill="none" />
            <path d="M 15,15.5 A 0.5,0.5 0 1,1 14,15.5 A 0.5,0.5 0 1,1 15,15.5 z" fill={isWhite ? "#262626" : "#ffffff"} />
          </g>
        </svg>
      );
    case 'b':
      return (
        <svg viewBox="0 0 45 45" className={className}>
          <g
            fill={isWhite ? "#ffffff" : "#262626"}
            stroke={isWhite ? "#262626" : "#ffffff"}
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <path d="M 9,36 C 9,36 22.5,37 22.5,29 C 22.5,31 25,31 25,30 C 25,28 22.5,26 22.5,21 C 22.5,15 27,8 22.5,8 C 18,8 22.5,15 22.5,21 C 22.5,26 20,28 20,30 C 20,31 22.5,31 22.5,29 C 22.5,37 9,36 9,36 z" />
            <circle cx="22.5" cy="5" r="2" />
            <path d="M 17.5,18 L 27.5,18" fill="none" />
            <path d="M 22.5,14 L 22.5,22" fill="none" />
          </g>
        </svg>
      );
    case 'q':
      return (
        <svg viewBox="0 0 45 45" className={className}>
          <g
            fill={isWhite ? "#ffffff" : "#262626"}
            stroke={isWhite ? "#262626" : "#ffffff"}
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <path d="M 8,40 A 1,1 0 0,0 9,41 L 36,41 A 1,1 0 0,0 37,40 L 38.5,15 L 30,27 L 22.5,10 L 15,27 L 6.5,15 L 8,40 z" />
            <circle cx="6" cy="12" r="2" />
            <circle cx="15" cy="21" r="2" />
            <circle cx="22.5" cy="9" r="2" />
            <circle cx="30" cy="21" r="2" />
            <circle cx="39" cy="12" r="2" />
          </g>
        </svg>
      );
    case 'k':
      return (
        <svg viewBox="0 0 45 45" className={className}>
          <g
            fill={isWhite ? "#ffffff" : "#262626"}
            stroke={isWhite ? "#262626" : "#ffffff"}
            strokeWidth="1.5"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <path d="M 8.5,14 L 36.5,14 L 36.5,9 L 8.5,9 L 8.5,14 z" />
            <path d="M 11.5,14 L 11.5,35 L 33.5,35 L 33.5,14 L 11.5,14 z" />
            <path d="M 11.5,35 L 8.5,41 L 36.5,41 L 33.5,35 L 11.5,35 z" />
            <path d="M 22.5,9 L 22.5,2" />
            <path d="M 19,5 L 26,5" />
            <path d="M 11.5,30 L 33.5,30" fill="none" />
            <path d="M 11.5,18 L 33.5,18" fill="none" />
          </g>
        </svg>
      );
    default:
      return null;
  }
};
