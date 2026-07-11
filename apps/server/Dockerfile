# Stage 1: Build the C++ Engine
FROM gcc:12 AS engine-builder
RUN apt-get update && apt-get install -y cmake make g++ && rm -rf /var/lib/apt/lists/*
WORKDIR /engine
COPY ./engine /engine
RUN mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc) chess_benchmarks

# Stage 2: Build the Node Express Server
FROM node:20 AS server-builder
WORKDIR /app
COPY package*.json ./
COPY ./shared ./shared
COPY ./apps/server ./apps/server
RUN npm ci --workspace=shared --workspace=apps/server
RUN npm run build -w shared
RUN npm run build -w apps/server

# Stage 3: Runner
FROM node:20-slim
RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*
WORKDIR /app

COPY --from=server-builder /app/package*.json ./
COPY --from=server-builder /app/node_modules ./node_modules
COPY --from=server-builder /app/shared ./shared
COPY --from=server-builder /app/apps/server/dist ./apps/server/dist
COPY --from=server-builder /app/apps/server/package.json ./apps/server/package.json
COPY --from=server-builder /app/apps/server/prisma ./apps/server/prisma

# Copy compiled C++ engine
COPY --from=engine-builder /engine/build/benchmarks/Release/chess_benchmarks /app/engine/build/benchmarks/Release/chess_benchmarks

ENV NODE_ENV=production
ENV ENGINE_PATH=/app/engine/build/benchmarks/Release/chess_benchmarks
ENV PORT=4000

EXPOSE 4000
CMD ["sh", "-c", "npx prisma db push && node apps/server/dist/index.js"]
