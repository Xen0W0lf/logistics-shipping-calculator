#include "ReportGenerator.h"
#include <fstream>
#include <iomanip>

void ReportGenerator::generateTextReport(const std::string& filepath, const std::vector<CalculationResult>& results) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "========================================\n";
    file << "       SHIPPING TARIFF REPORT\n";
    file << "========================================\n\n";
    
    double total_cost = 0;
    for (const auto& res : results) {
        if (res.cost < 0) continue;
        file << "Request ID : " << res.request_id << "\n";
        file << "Route      : " << res.origin_city << " -> " << res.destination_city << "\n";
        file << "Distance   : " << std::fixed << std::setprecision(2) << res.distance_km << " km\n";
        file << "Cost       : $" << std::fixed << std::setprecision(2) << res.cost << "\n";
        file << "----------------------------------------\n";
        total_cost += res.cost;
    }
    file << "Total Cost : $" << std::fixed << std::setprecision(2) << total_cost << "\n";
    file << "========================================\n";
}

void ReportGenerator::generateHTMLMap(const std::string& filepath, const DataReader& data) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Logistics Tracker - Interactive</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
    <style>
        :root { --bg: #0f172a; --panel: rgba(30, 41, 59, 0.85); --text: #f8fafc; --accent: #3b82f6; --accent-glow: rgba(59, 130, 246, 0.5); }
        body { margin: 0; padding: 0; font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); overflow: hidden; }
        #map { width: 100vw; height: 100vh; z-index: 1; }
        .overlay { position: absolute; top: 20px; left: 20px; z-index: 1000; width: 350px; background: var(--panel); backdrop-filter: blur(12px); border-radius: 16px; border: 1px solid rgba(255,255,255,0.1); padding: 24px; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); max-height: 85vh; overflow-y: auto; }
        .overlay::-webkit-scrollbar { width: 6px; }
        .overlay::-webkit-scrollbar-track { background: transparent; }
        .overlay::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.2); border-radius: 3px; }
        h1 { margin: 0 0 20px 0; font-size: 22px; font-weight: 800; }
        label { font-size: 14px; font-weight: 600; color: #cbd5e1; margin-bottom: 4px; display: block; }
        select, input[type="number"] { width: 100%; padding: 10px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.2); background: rgba(0,0,0,0.3); color: white; margin-bottom: 12px; font-family: 'Inter', sans-serif; box-sizing: border-box; }
        select option { background: var(--bg); color: white; }
        button { width: 100%; padding: 12px; border-radius: 8px; border: none; background: var(--accent); color: white; font-weight: 800; font-size: 16px; cursor: pointer; transition: all 0.2s ease; margin-top: 10px; }
        button:hover { background: #2563eb; transform: translateY(-1px); box-shadow: 0 4px 12px var(--accent-glow); }
        .checkbox-group { display: flex; gap: 15px; margin-bottom: 15px; }
        .checkbox-group label { display: flex; align-items: center; gap: 6px; cursor: pointer; margin: 0; font-weight: 400; }
        .shipment { background: rgba(0,0,0,0.4); border-radius: 12px; padding: 16px; margin-top: 20px; border: 1px solid var(--accent); box-shadow: 0 0 20px var(--accent-glow); display: none; }
        .cost { font-size: 24px; font-weight: 800; color: #10b981; margin-bottom: 8px; }
        .route { font-size: 16px; font-weight: 600; margin-bottom: 4px; }
        .leaflet-container { background: #0f172a !important; }
        .package-icon { background: none; border: none; }
        .package-dot { width: 16px; height: 16px; background: #3b82f6; border-radius: 50%; box-shadow: 0 0 15px #3b82f6, 0 0 30px #3b82f6; border: 2px solid white; }
    </style>
</head>
<body>
    <div id="map"></div>
    <div class="overlay">
        <h1>📦 Create Shipment</h1>
        
        <label>Origin City</label>
        <select id="origin">
)HTML";

    const auto& locs = data.getLocations();
    for (const auto& [city, loc] : locs) {
        file << "            <option value=\"" << city << "\">" << city << ", " << loc.state << "</option>\n";
    }

    file << R"HTML(        </select>
        
        <label>Destination City</label>
        <select id="dest">
)HTML";

    for (const auto& [city, loc] : locs) {
        file << "            <option value=\"" << city << "\">" << city << ", " << loc.state << "</option>\n";
    }

    file << R"HTML(        </select>
        
        <label>Weight (kg)</label>
        <input type="number" id="weight" value="10" min="0.1" step="0.1">
        
        <div class="checkbox-group">
            <label><input type="checkbox" id="express"> Express</label>
            <label><input type="checkbox" id="fragile"> Fragile</label>
        </div>
        
        <button onclick="calculateTariff()">Calculate & Route</button>

        <div id="result-box" class="shipment">
            <div class="cost" id="res-cost">$0.00</div>
            <div class="route" id="res-route">City ➔ City</div>
            <div style="font-size: 14px; color: #cbd5e1;" id="res-dist">Distance: 0 km</div>
        </div>
    </div>
    
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script>
        const map = L.map('map', {zoomControl: false}).setView([39.8283, -98.5795], 4);
        L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
            attribution: '&copy; <a href="https://carto.com/">CARTO</a>'
        }).addTo(map);
        
        let currentPath = null;
        let currentMarker = null;
        let animationInterval = null;

        const packageIcon = L.divIcon({
            className: 'package-icon',
            html: '<div class="package-dot"></div>',
            iconSize: [16, 16],
            iconAnchor: [8, 8]
        });

        async function calculateTariff() {
            const payload = {
                id: "DYN-" + Math.floor(Math.random()*1000),
                origin_city: document.getElementById('origin').value,
                destination_city: document.getElementById('dest').value,
                weight_kg: parseFloat(document.getElementById('weight').value),
                is_express: document.getElementById('express').checked,
                is_fragile: document.getElementById('fragile').checked
            };
            
            try {
                const response = await fetch('/api/calculate', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                
                const data = await response.json();
                
                if (data.error) {
                    alert("Error: " + data.error);
                    return;
                }
                
                document.getElementById('result-box').style.display = 'block';
                document.getElementById('res-cost').innerText = '$' + data.cost.toFixed(2);
                document.getElementById('res-route').innerText = data.origin_city + ' ➔ ' + data.destination_city;
                document.getElementById('res-dist').innerText = 'Distance: ' + data.distance_km.toFixed(0) + ' km';
                
                animateRoute(data.origin_coord, data.dest_coord);
            } catch (err) {
                alert("Failed to connect to local server.");
            }
        }
        
        function animateRoute(origin, dest) {
            if (currentPath) map.removeLayer(currentPath);
            if (currentMarker) map.removeLayer(currentMarker);
            if (animationInterval) clearInterval(animationInterval);

            currentPath = L.polyline([origin, dest], {
                color: '#3b82f6',
                weight: 3,
                opacity: 0.5,
                dashArray: '10, 10',
                lineCap: 'round'
            }).addTo(map);

            map.fitBounds(currentPath.getBounds(), { padding: [100, 100] });

            currentMarker = L.marker(origin, { icon: packageIcon }).addTo(map);

            let progress = 0;
            const steps = 150;
            const intervalTime = 16;

            animationInterval = setInterval(() => {
                progress += 1;
                if (progress > steps) progress = 0;
                
                const t = progress / steps;
                const easeT = t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
                
                const lat = origin[0] + (dest[0] - origin[0]) * easeT;
                const lng = origin[1] + (dest[1] - origin[1]) * easeT;
                
                currentMarker.setLatLng([lat, lng]);
            }, intervalTime);
        }
    </script>
</body>
</html>)HTML";
}
