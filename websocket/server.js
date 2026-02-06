const WebSocket = require('ws');
const si = require('systeminformation'); // Everything but temp works on Windows, use Ubuntu/Linux

const wss = new WebSocket.Server({ port: 8080 })
console.log('↬ WebSocket server started on port http://localhost:8080')

setInterval(async () => {
    try {
        const cpuLoad = await si.currentLoad();
        const mem = await si.mem();
        const temp = await si.cpuTemperature();

        const stats = JSON.stringify({
            cpu: Math.round(cpuLoad.currentLoad),
            mem: Math.round((mem.active / mem.total) * 100),
            temp: Math.round(temp.main || 0),
            uptime: Math.round(si.time().uptime)
        });

        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(stats);
            }
        });
    } catch (error) {
        console.error("Error fetching system stats:", error);
    }
}, 1000);

wss.on('connection', (ws, req) => {
    const ip = req.socket.remoteAddress;
    console.log(`[${new Date().toLocaleTimeString()}] Client connected from: ${ip}`);
    
    ws.on('close', () => {
        console.log(`[${new Date().toLocaleTimeString()}] Client disconnected.`);
    });
});
