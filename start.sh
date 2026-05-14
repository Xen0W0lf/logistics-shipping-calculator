#!/bin/bash
echo "Starting DataService..."
./DataService &

echo "Starting CalculationService..."
./CalculationService &

# Wait a brief moment to ensure backend services are up
sleep 2

echo "Starting FrontendGateway..."
# The FrontendGateway will bind to $PORT
exec ./FrontendGateway
