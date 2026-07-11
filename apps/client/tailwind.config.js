/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        darkBg: '#090a0f',
        panelBg: '#12141c',
        cardBg: '#181b26',
        accentBg: '#1e2230',
        primary: '#4f46e5', // Indigo-600
        primaryLight: '#6366f1', // Indigo-500
        secondary: '#10b981', // Emerald-500
        danger: '#ef4444', // Red-500
        textColor: '#e2e8f0', // Slate-200
        textMuted: '#94a3b8', // Slate-400
        borderColor: '#242938'
      },
      fontFamily: {
        sans: ['Inter', 'sans-serif'],
        mono: ['JetBrains Mono', 'Fira Code', 'monospace']
      }
    },
  },
  plugins: [],
}
