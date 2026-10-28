# Build the static files with Vite, then serve them with nginx{% if has_backend %}, which also forwards /api to the backend{% endif %}.
FROM node:{% v_node %}-alpine AS build
WORKDIR /app
COPY package*.json ./
RUN npm install
COPY . .
RUN npm run build

FROM nginx:stable-alpine
COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /app/dist /usr/share/nginx/html
EXPOSE 80
