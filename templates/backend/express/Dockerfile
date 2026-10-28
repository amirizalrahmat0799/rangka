# Build the TypeScript, then run the compiled JavaScript with production dependencies only.
FROM node:{% v_node %}-alpine AS build
WORKDIR /app
COPY package*.json ./
RUN npm install
COPY . .
RUN npm run build

FROM node:{% v_node %}-alpine
WORKDIR /app
ENV NODE_ENV=production
COPY package*.json ./
RUN npm install --omit=dev && npm cache clean --force
COPY --from=build /app/dist ./dist
{% if postgres %}
COPY migrations ./migrations
{% endif %}
USER node
EXPOSE 8080
CMD ["node", "dist/server.js"]
