FROM ubuntu:22.04

# Avoid tzdata interactive prompt during package installation
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code and data files
COPY . .

# Compile microservices for Linux (-pthread instead of -lws2_32)
RUN g++ src/services/DataService.cpp src/DataReader.cpp -o DataService -Iinclude -std=c++17 -pthread
RUN g++ src/services/CalculationService.cpp -o CalculationService -Iinclude -std=c++17 -pthread
RUN g++ src/services/FrontendGateway.cpp -o FrontendGateway -Iinclude -std=c++17 -pthread

# Make start script executable
RUN chmod +x start.sh

# The frontend gateway dynamically listens to $PORT
# Expose default port just in case (Render ignores EXPOSE and uses $PORT anyway)
EXPOSE 8080

CMD ["./start.sh"]
