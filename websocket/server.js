const WebSocket = require('ws');
const si = require('systeminformation'); // Everything but temp works on Windows, use Ubuntu/Linux
const wss = new WebSocket.Server({ port: 8080 });

console.log('↬ WebSocket server started on port wss://localhost:8080')

setInterval(async () => {
    try {
        const cpuLoad = await si.currentLoad();
        const mem = await si.mem();
        const temp = await si.cpuTemperature();
        const time = si.time();
        const disk = await si.fsSize();
        const net = await si.networkStats();

        const rootDisk = disk.find(d => d.mount === '/') || disk[0];
        const netData = net[0];

        const stats = JSON.stringify({
            cpu: Math.round(cpuLoad.currentLoad),
            mem: Math.round((mem.active / mem.total) * 100),
            temp: Math.round(temp.main || 0),
            uptime: Math.round(time.uptime),
            disk: Math.round(rootDisk.use),
            netIn: Math.round(netData.rx_sec / 1024), // KB/s
            netOut: Math.round(netData.tx_sec / 1024) // KB/s
        });

        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(stats);
            }
        });
    } catch (error) {
        console.error("Error fetching stats:", error);
    }
}, 1000);

wss.on('connection', (ws) => {
    console.log("Client connected");
});
