#define WIN32_LEAN_AND_MEAN
#include "../include/httplib.h"
#include "../include/json.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <fstream>

using json = nlohmann::json;

json fetchServiceJson(const std::string& host, int port, const std::string& path) {
    httplib::Client cli(host, port);
    cli.set_connection_timeout(0, 500000); // 500ms
    cli.set_read_timeout(1, 0); // 1s
    if (auto res = cli.Get(path)) {
        if (res->status == 200) {
            return json::parse(res->body);
        }
    }
    return json::object();
}

json postServiceJson(const std::string& host, int port, const std::string& path, const json& body) {
    httplib::Client cli(host, port);
    if (auto res = cli.Post(path, body.dump(), "application/json")) {
        if (res->status == 200) {
            return json::parse(res->body);
        }
    }
    return json::object();
}

std::string generateHTML() {
    std::string html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Global Logistics Routing</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
    <style>
        :root { --bg: #111827; --panel: rgba(31, 41, 55, 0.95); --text: #f3f4f6; --accent: #3b82f6; --accent-glow: rgba(59, 130, 246, 0.5); --success: #10b981; }
        body { margin: 0; padding: 0; font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); overflow: hidden; }
        #map { width: 100vw; height: 100vh; z-index: 1; }
        .overlay { position: absolute; top: 20px; left: 20px; z-index: 1000; width: 400px; background: var(--panel); backdrop-filter: blur(12px); border-radius: 16px; border: 1px solid rgba(255,255,255,0.1); padding: 24px; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); max-height: 85vh; overflow-y: visible; }
        h1 { margin: 0 0 10px 0; font-size: 22px; font-weight: 800; }
        .badge { display: inline-block; background: var(--success); color: #000; font-size: 11px; font-weight: 800; padding: 3px 8px; border-radius: 12px; margin-bottom: 20px; }
        label { font-size: 14px; font-weight: 600; color: #cbd5e1; margin-bottom: 4px; display: block; }
        input[type="text"], input[type="number"], select { width: 100%; padding: 10px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.2); background: rgba(0,0,0,0.3); color: white; margin-bottom: 12px; font-family: 'Inter', sans-serif; box-sizing: border-box; }
        button { width: 100%; padding: 12px; border-radius: 8px; border: none; background: var(--accent); color: white; font-weight: 800; font-size: 16px; cursor: pointer; transition: all 0.2s ease; margin-top: 10px; }
        button:hover { background: #ea580c; transform: translateY(-1px); box-shadow: 0 4px 12px var(--accent-glow); }
        .checkbox-group { display: flex; gap: 15px; margin-bottom: 15px; }
        .checkbox-group label { display: flex; align-items: center; gap: 6px; cursor: pointer; margin: 0; font-weight: 400; }
        .shipment { background: rgba(0,0,0,0.4); border-radius: 12px; padding: 16px; margin-top: 20px; border: 1px solid var(--accent); box-shadow: 0 0 20px var(--accent-glow); display: none; }
        .cost { font-size: 24px; font-weight: 800; color: var(--success); margin-bottom: 8px; }
        .route-info { font-size: 14px; color: #cbd5e1; margin-bottom: 4px; }
        .leaflet-container { background: #0f172a !important; }
        .loading { display: none; text-align: center; margin-top: 10px; color: var(--accent); font-weight: bold; }
        .suggestions-box { position: absolute; top: 65px; left: 0; width: 100%; background: #1e293b; border: 1px solid rgba(255,255,255,0.2); border-radius: 8px; z-index: 2000; display: none; max-height: 200px; overflow-y: auto; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
        .suggestion-item { padding: 10px; cursor: pointer; font-size: 13px; color: #cbd5e1; border-bottom: 1px solid rgba(255,255,255,0.05); }
        .suggestion-item:hover { background: var(--accent); color: white; }
        @keyframes dash { to { stroke-dashoffset: -20; } }
        .animated-line { animation: dash 1s linear infinite; }
    </style>
</head>
<body>
    <div id="map"></div>
    <div class="overlay">
        <h1>🌍 Global Logistics Routing</h1>
        <div class="badge">AUTO HUB ROUTING</div>
        
        <div style="position: relative;">
            <label>Origin</label>
            <input type="text" id="origin" value="" placeholder="e.g. New York" autocomplete="off" oninput="searchLocation('origin', this.value)">
            <div id="origin-suggestions" class="suggestions-box"></div>
        </div>
        
        <div style="position: relative;">
            <label>Destination</label>
            <input type="text" id="dest" value="" placeholder="e.g. Sibiu" autocomplete="off" oninput="searchLocation('dest', this.value)">
            <div id="dest-suggestions" class="suggestions-box"></div>
        </div>
        
        <label>Weight (kg)</label>
        <input type="number" id="weight" value="10" min="0.1" step="0.1">
        
        <div class="checkbox-group">
            <label><input type="checkbox" id="express"> Express</label>
            <label><input type="checkbox" id="fragile"> Fragile</label>
        </div>
        
        <button onclick="calculateRoute()">Calculate Route & Tariff</button>
        <div id="loading" class="loading">Routing & Geocoding... Please wait.</div>

        <div id="result-box" class="shipment">
            <div class="cost" id="res-cost">$0.00</div>
            <div class="route-info" id="res-dist">Total Distance: 0 km</div>
            <div class="route-info" id="res-details" style="font-size:12px; margin-top:10px;"></div>
        </div>
    </div>
    
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    
    <script>
        const map = L.map('map', {zoomControl: false}).setView([39.8283, -98.5795], 3);
        L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
            attribution: '&copy; CARTO'
        }).addTo(map);
        
        let currentLayers = [];

        const GLOBAL_HUBS = [
            { name: "JFK Airport, New York", lat: 40.6413, lon: -73.7781 },
            { name: "Amsterdam Schiphol", lat: 52.3105, lon: 4.7683 },
            { name: "LAX Airport, Los Angeles", lat: 33.9416, lon: -118.4085 },
            { name: "Narita Airport, Tokyo", lat: 35.7719, lon: 140.3928 },
            { name: "London Heathrow", lat: 51.4700, lon: -0.4543 }
        ];

        let debounceTimer;
        async function searchLocation(inputId, query) {
            const box = document.getElementById(inputId + '-suggestions');
            document.getElementById(inputId).removeAttribute('data-lat');
            document.getElementById(inputId).removeAttribute('data-lon');
            if (query.length < 3) { box.style.display = 'none'; return; }
            
            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(async () => {
                try {
                    const res = await fetch(`https://nominatim.openstreetmap.org/search?q=${encodeURIComponent(query)}&format=json&limit=5`);
                    const data = await res.json();
                    box.innerHTML = '';
                    if (data.length > 0) {
                        data.forEach(item => {
                            const div = document.createElement('div');
                            div.className = 'suggestion-item';
                            div.innerText = item.display_name;
                            div.onclick = () => {
                                document.getElementById(inputId).value = item.display_name;
                                document.getElementById(inputId).setAttribute('data-lat', item.lat);
                                document.getElementById(inputId).setAttribute('data-lon', item.lon);
                                box.style.display = 'none';
                            };
                            box.appendChild(div);
                        });
                        box.style.display = 'block';
                    } else {
                        box.style.display = 'none';
                    }
                } catch(e) { console.error(e); }
            }, 400);
        }

        document.addEventListener('click', function(e) {
            if (!e.target.closest('#origin') && !e.target.closest('#origin-suggestions')) {
                document.getElementById('origin-suggestions').style.display = 'none';
            }
            if (!e.target.closest('#dest') && !e.target.closest('#dest-suggestions')) {
                document.getElementById('dest-suggestions').style.display = 'none';
            }
        });

        function getClosestHub(lat, lon) {
            let minD = Infinity;
            let closest = null;
            for (let h of GLOBAL_HUBS) {
                let d = calcStraightDistance(lat, lon, h.lat, h.lon);
                if (d < minD) { minD = d; closest = h; }
            }
            return closest;
        }
        
        async function geocode(query) {
            const res = await fetch(`https://nominatim.openstreetmap.org/search?q=${encodeURIComponent(query)}&format=json&limit=1`);
            const data = await res.json();
            if (data.length === 0) throw new Error(`Could not find location: ${query}`);
            return { lat: parseFloat(data[0].lat), lon: parseFloat(data[0].lon), display_name: data[0].display_name };
        }
        
        function calcStraightDistance(lat1, lon1, lat2, lon2) {
            const R = 6371;
            const dLat = (lat2 - lat1) * Math.PI / 180;
            const dLon = (lon2 - lon1) * Math.PI / 180;
            const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
                      Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
                      Math.sin(dLon/2) * Math.sin(dLon/2);
            const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
            return R * c;
        }

        async function getRouteData(p1, p2, mode) {
            if (mode === 'car') {
                try {
                    const res = await fetch(`https://router.project-osrm.org/route/v1/driving/${p1.lon},${p1.lat};${p2.lon},${p2.lat}?geometries=geojson&overview=full`);
                    const data = await res.json();
                    if (data.code === 'Ok') {
                        if (data.waypoints[0].distance > 50000 || data.waypoints[1].distance > 50000) {
                            return { success: false };
                        }
                        return {
                            success: true,
                            geometry: data.routes[0].geometry,
                            distance_km: data.routes[0].distance / 1000
                        };
                    }
                } catch(e) { console.warn("OSRM failed"); }
            }
            return { success: false };
        }

        async function calculateRoute() {
            document.getElementById('loading').style.display = 'block';
            document.getElementById('result-box').style.display = 'none';
            
            currentLayers.forEach(l => map.removeLayer(l));
            currentLayers = [];
            
            try {
                const originInput = document.getElementById('origin');
                const destInput = document.getElementById('dest');
                
                if (!originInput.value.trim() || !destInput.value.trim()) {
                    throw new Error("Please fill in Origin and Destination.");
                }
                
                let originData;
                if (originInput.hasAttribute('data-lat')) {
                    originData = { lat: parseFloat(originInput.getAttribute('data-lat')), lon: parseFloat(originInput.getAttribute('data-lon')), display_name: originInput.value };
                } else {
                    originData = await geocode(originInput.value);
                    originInput.value = originData.display_name;
                    originInput.setAttribute('data-lat', originData.lat);
                    originInput.setAttribute('data-lon', originData.lon);
                }
                
                let destData;
                if (destInput.hasAttribute('data-lat')) {
                    destData = { lat: parseFloat(destInput.getAttribute('data-lat')), lon: parseFloat(destInput.getAttribute('data-lon')), display_name: destInput.value };
                } else {
                    destData = await geocode(destInput.value);
                    destInput.value = destData.display_name;
                    destInput.setAttribute('data-lat', destData.lat);
                    destInput.setAttribute('data-lon', destData.lon);
                }
                
                let bounds = L.latLngBounds();
                const blueDistKm = calcStraightDistance(originData.lat, originData.lon, destData.lat, destData.lon);
                
                // Draw Blue Line
                const blueLine = L.polyline([[originData.lat, originData.lon], [destData.lat, destData.lon]], {
                    color: '#3b82f6', weight: 4, opacity: 0.8, dashArray: '10, 10', className: 'animated-line'
                }).addTo(map);
                currentLayers.push(blueLine);
                bounds.extend(blueLine.getBounds());
                
                let legsData = [];
                let routeGeometries = [];
                
                // Attempt Direct Car Route
                const directRoute = await getRouteData(originData, destData, 'car');
                if (directRoute.success) {
                    legsData.push({ mode: 'car', distance_km: directRoute.distance_km });
                    routeGeometries.push(directRoute.geometry);
                } else {
                    // Hub Routing
                    const hubA = getClosestHub(originData.lat, originData.lon);
                    const hubB = getClosestHub(destData.lat, destData.lon);
                    
                    if (hubA === hubB) {
                        legsData.push({ mode: 'airplane', distance_km: calcStraightDistance(originData.lat, originData.lon, destData.lat, destData.lon) });
                        routeGeometries.push({ type: "LineString", coordinates: [[originData.lon, originData.lat], [destData.lon, destData.lat]] });
                    } else {
                        // Leg 1: Origin to HubA (Car)
                        const leg1 = await getRouteData(originData, hubA, 'car');
                        if (leg1.success) {
                            legsData.push({ mode: 'car', distance_km: leg1.distance_km });
                            routeGeometries.push(leg1.geometry);
                        } else {
                            legsData.push({ mode: 'airplane', distance_km: calcStraightDistance(originData.lat, originData.lon, hubA.lat, hubA.lon) });
                            routeGeometries.push({ type: "LineString", coordinates: [[originData.lon, originData.lat], [hubA.lon, hubA.lat]] });
                        }
                        
                        // Leg 2: HubA to HubB (Airplane)
                        legsData.push({ mode: 'airplane', distance_km: calcStraightDistance(hubA.lat, hubA.lon, hubB.lat, hubB.lon) });
                        routeGeometries.push({ type: "LineString", coordinates: [[hubA.lon, hubA.lat], [hubB.lon, hubB.lat]] });
                        
                        // Leg 3: HubB to Dest (Car)
                        const leg3 = await getRouteData(hubB, destData, 'car');
                        if (leg3.success) {
                            legsData.push({ mode: 'car', distance_km: leg3.distance_km });
                            routeGeometries.push(leg3.geometry);
                        } else {
                            legsData.push({ mode: 'airplane', distance_km: calcStraightDistance(hubB.lat, hubB.lon, destData.lat, destData.lon) });
                            routeGeometries.push({ type: "LineString", coordinates: [[hubB.lon, hubB.lat], [destData.lon, destData.lat]] });
                        }
                    }
                }
                
                // Draw Orange Lines
                routeGeometries.forEach(geom => {
                    const geojsonLayer = L.geoJSON(geom, {
                        style: { color: '#f97316', weight: 6, opacity: 1.0 }
                    }).addTo(map);
                    currentLayers.push(geojsonLayer);
                    bounds.extend(geojsonLayer.getBounds());
                });
                
                // Markers
                const startMarker = L.circleMarker([originData.lat, originData.lon], { radius: 6, fillColor: '#10b981', color: '#fff', weight: 2, fillOpacity: 1 }).addTo(map).bindPopup("Origin");
                const endMarker = L.circleMarker([destData.lat, destData.lon], { radius: 6, fillColor: '#ef4444', color: '#fff', weight: 2, fillOpacity: 1 }).addTo(map).bindPopup("Destination");
                currentLayers.push(startMarker, endMarker);
                
                map.fitBounds(bounds, { padding: [50, 50] });
                
                const payload = {
                    legs: legsData,
                    weight_kg: parseFloat(document.getElementById('weight').value),
                    is_express: document.getElementById('express').checked,
                    is_fragile: document.getElementById('fragile').checked
                };
                
                const apiUrl = window.location.protocol === 'file:' ? 'http://127.0.0.1:8080/api/calculate' : '/api/calculate';
                
                const response = await fetch(apiUrl, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                
                const data = await response.json();
                if (data.error) throw new Error(data.error);
                
                document.getElementById('result-box').style.display = 'block';
                document.getElementById('res-cost').innerText = '$' + data.cost.toFixed(2);
                document.getElementById('res-dist').innerText = 'Total Distance: ' + data.distance_km.toFixed(0) + ' km';
                
                let detailsHtml = `<span style="color: #3b82f6; font-weight: bold;">DIRECT (BLUE LINE): ${blueDistKm.toFixed(0)} km</span><br>`;
                for(let m in data.mode_breakdown) {
                    if (data.mode_breakdown[m] > 0) {
                        detailsHtml += `${m.toUpperCase()}: ${data.mode_breakdown[m].toFixed(0)} km<br>`;
                    }
                }
                document.getElementById('res-details').innerHTML = detailsHtml;

            } catch (err) {
                alert("Error: " + err.message);
            } finally {
                document.getElementById('loading').style.display = 'none';
            }
        }
    </script>
</body>
</html>)HTML";
    return html;
}

int main() {
    std::cout << "[FrontendGateway] Starting on port 8080...\n";
    
    std::string index_html = generateHTML();
    std::ofstream out_html("index.html");
    out_html << index_html;
    out_html.close();
    std::cout << "[FrontendGateway] Exported local index.html\n";

    httplib::Server svr;

    svr.Get("/", [&index_html](const httplib::Request&, httplib::Response& res) {
        res.set_content(index_html, "text/html");
    });

    svr.Options("/api/calculate", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    svr.Post("/api/calculate", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        try {
            auto j = json::parse(req.body);
            
            json rates = fetchServiceJson("localhost", 8081, "/rates");
            if (rates.empty()) {
                rates["base_rate_per_kg"] = 0.5;
                rates["express_multiplier"] = 1.5;
                rates["fragile_multiplier"] = 1.2;
            }
            
            json calc_req = j;
            calc_req["base_rate"] = rates["base_rate_per_kg"];
            calc_req["express_multiplier"] = rates["express_multiplier"];
            calc_req["fragile_multiplier"] = rates["fragile_multiplier"];
            calc_req["origin_region_multiplier"] = 1.0;
            calc_req["dest_region_multiplier"] = 1.0;
            
            json calc_res = postServiceJson("localhost", 8082, "/compute", calc_req);
            
            if (calc_res.empty() || calc_res.contains("error")) {
                res.set_content(R"({"error": "CalculationService failed"})", "application/json");
                return;
            }
            
            res.set_content(calc_res.dump(), "application/json");
        } catch (std::exception& e) {
            res.set_content(R"({"error": "Gateway Processing Error"})", "application/json");
        }
    });

    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }

    std::cout << "[FrontendGateway] Listening on all network interfaces (http://0.0.0.0:" << port << ")\n";
    svr.listen("0.0.0.0", port);
    return 0;
}
