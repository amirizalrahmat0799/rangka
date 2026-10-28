# Build the jar with Maven, then run it on a JRE-only image.
FROM maven:3.9-eclipse-temurin-{% v_java %} AS build
WORKDIR /src
COPY pom.xml .
RUN mvn -B -q dependency:go-offline
COPY src src
RUN mvn -B -q package -DskipTests

FROM eclipse-temurin:{% v_java %}-jre
WORKDIR /app
COPY --from=build /src/target/app.jar app.jar
RUN useradd --system appuser{% if h2 %} && mkdir data && chown appuser data{% endif %}
USER appuser
EXPOSE 8080
ENTRYPOINT ["java", "-jar", "app.jar"]
