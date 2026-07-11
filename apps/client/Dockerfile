# Stage 1: Build the React Application
FROM node:20 AS client-builder
WORKDIR /app
COPY package*.json ./
COPY ./shared ./shared
COPY ./apps/client ./apps/client
RUN npm ci --workspace=shared --workspace=apps/client
RUN npm run build -w shared
RUN npm run build -w apps/client

# Stage 2: Serve with Nginx
FROM nginx:alpine
COPY --from=client-builder /app/apps/client/dist /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
