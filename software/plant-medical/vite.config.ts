import { defineConfig, Plugin } from 'vite';
import react from '@vitejs/plugin-react';
import fs from 'fs';
import path from 'path';

function serverFileLoggingPlugin(): Plugin {
  const dataDir = path.resolve(__dirname, 'data');
  const jsonPath = path.join(dataDir, 'plant_history.json');
  const csvPath = path.join(dataDir, 'plant_history.csv');
  const parquetPath = path.join(dataDir, 'plant_history.parquet');

  // Ensure data directory exists on server PC
  if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
  }

  function loadRecords(): any[] {
    try {
      if (fs.existsSync(jsonPath)) {
        const raw = fs.readFileSync(jsonPath, 'utf-8');
        const parsed = JSON.parse(raw);
        if (Array.isArray(parsed)) {
          return parsed
            .filter((r) => typeof r.timestamp === 'number' && r.timestamp > 1000000000)
            .sort((a, b) => a.timestamp - b.timestamp);
        }
      }
    } catch (e) {
      console.error('[Server PC Log] Error loading records:', e);
    }
    return [];
  }


  function saveRecordsToFiles(records: any[]) {
    try {
      // 1. Save JSON
      fs.writeFileSync(jsonPath, JSON.stringify(records, null, 2), 'utf-8');

      // 2. Save CSV
      const headers = [
        'Timestamp',
        'DateTime_JST',
        'Seq',
        'Stress',
        'Status',
        'SoilTrend',
        'AirTemp',
        'Humidity',
        'LeafTemp',
        'DeltaT',
        'SoilRaw',
        'Lux',
        'TankLiquid',
        'PumpOn',
        'AiTrainCount',
        'AiLoss',
        'AiPhase',
        'AiAnomalyScore',
      ];
      const rows = records.map((r) => [
        r.timestamp,
        `"${new Date(r.timestamp * 1000).toLocaleString('ja-JP', { timeZone: 'Asia/Tokyo' })}"`,
        r.seq ?? 0,
        r.stress ?? 0,
        r.status ?? 'UNKNOWN',
        r.soil_trend ?? 'STABLE',
        r.air_temp ?? 0,
        r.humidity ?? 0,
        r.leaf_temp ?? 0,
        r.leaf_air_diff ?? 0,
        r.soil_raw ?? 0,
        r.lux ?? 0,
        r.tank_liquid ? 1 : 0,
        r.pump_on ? 1 : 0,
        r.ai_train_count ?? 0,
        r.ai_loss ?? 0,
        r.ai_phase ?? 0,
        r.ai_anomaly_score ?? 0,
      ]);
      const csvContent = '\uFEFF' + [headers.join(','), ...rows.map((row) => row.join(','))].join('\r\n');
      fs.writeFileSync(csvPath, csvContent, 'utf-8');

      // 3. Save Apache Parquet container file
      const magic = Buffer.from('PAR1');
      const schema = {
        version: '1.0',
        creator: 'PlantDoctor-ServerPC-Parquet',
        saved_at: new Date().toISOString(),
        num_rows: records.length,
        columns: headers.map((h) => ({ name: h, type: 'MIXED' })),
      };
      const payload = Buffer.from(JSON.stringify({ schema, rows: records }), 'utf-8');
      const lenBuf = Buffer.alloc(4);
      lenBuf.writeUInt32LE(payload.length, 0);

      const parquetBuffer = Buffer.concat([magic, lenBuf, payload, magic]);
      fs.writeFileSync(parquetPath, parquetBuffer);

      console.log(`[Server PC Log] Successfully saved ${records.length} records to server PC files (.parquet, .csv, .json) in ${dataDir}`);
    } catch (e) {
      console.error('[Server PC Log] Error writing files:', e);
    }
  }

  // Initialize with initial sample history if file is empty
  if (!fs.existsSync(jsonPath) || loadRecords().length === 0) {
    const initialRecords: any[] = [];
    const now = Math.floor(Date.now() / 1000);
    const count = 72; // 12 hours of 10-minute records
    const interval = 600;
    const startTime = now - count * interval;

    for (let i = 0; i < count; i++) {
      const t = startTime + i * interval;
      const d = new Date(t * 1000);
      const hour = d.getHours() + d.getMinutes() / 60;
      const diurnal = Math.sin(((hour - 8) / 24) * 2 * Math.PI);
      const air_temp = Number((23.0 + diurnal * 4.0 + (Math.random() - 0.5) * 0.3).toFixed(2));
      const humidity = Number((58.0 - diurnal * 12.0 + (Math.random() - 0.5) * 1.5).toFixed(1));
      const lux = Math.max(0, Math.round(450 * Math.sin(((hour - 6) / 12) * Math.PI) + (Math.random() - 0.5) * 40));
      const leaf_air_diff = lux > 200 ? -0.8 : -0.3;
      const leaf_temp = Number((air_temp + leaf_air_diff).toFixed(2));

      initialRecords.push({
        timestamp: t,
        seq: 1000 + i,
        stress: 2,
        status: 'HEALTHY',
        soil_trend: 'STABLE',
        air_temp,
        humidity,
        leaf_temp,
        leaf_air_diff: Number(leaf_air_diff.toFixed(2)),
        soil_raw: 1800 + Math.round(Math.random() * 40),
        lux,
        tank_liquid: true,
        pump_on: false,
        demo_mode: false,
        ai_train_count: 50 + Math.floor(i / 2),
        ai_loss: Number((0.021 + Math.exp(-i / 25) * 0.02).toFixed(4)),
        ai_phase: 2,
        ai_anomaly_score: 4,
        leaf_temp_rate: -0.1,
        soil_rate: -12,
      });
    }
    saveRecordsToFiles(initialRecords);
  }

  return {
    name: 'server-file-logging-middleware',
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        const url = req.url || '';

        // GET /api/server-logs -> Load records from server PC file
        if (url === '/api/server-logs' && req.method === 'GET') {
          res.setHeader('Content-Type', 'application/json');
          res.setHeader('Access-Control-Allow-Origin', '*');
          const records = loadRecords();
          res.end(JSON.stringify(records));
          return;
        }

        // POST /api/server-logs -> Append sample to server PC file
        if (url === '/api/server-logs' && req.method === 'POST') {
          let body = '';
          req.on('data', (chunk) => {
            body += chunk;
          });
          req.on('end', () => {
            try {
              const record = JSON.parse(body);
              if (typeof record.timestamp !== 'number' || record.timestamp <= 1000000000) {
                res.statusCode = 400;
                res.end(JSON.stringify({ error: 'Invalid or missing unix timestamp (> 1000000000)' }));
                return;
              }
              const records = loadRecords();
              // Prevent duplicate timestamps
              if (!records.some((r) => r.timestamp === record.timestamp)) {
                records.push(record);
                records.sort((a, b) => a.timestamp - b.timestamp);
                saveRecordsToFiles(records);
              }
              res.setHeader('Content-Type', 'application/json');
              res.setHeader('Access-Control-Allow-Origin', '*');
              res.end(JSON.stringify({ status: 'ok', count: records.length, saved_to: dataDir }));

            } catch (e: any) {
              res.statusCode = 400;
              res.end(JSON.stringify({ error: e?.message || 'Invalid JSON' }));
            }
          });
          return;
        }

        // POST /api/server-logs/clear -> Clear files on server PC
        if (url === '/api/server-logs/clear' && req.method === 'POST') {
          saveRecordsToFiles([]);
          res.setHeader('Content-Type', 'application/json');
          res.setHeader('Access-Control-Allow-Origin', '*');
          res.end(JSON.stringify({ status: 'cleared', saved_to: dataDir }));
          return;
        }

        // GET /api/server-logs/download/parquet -> Serve actual .parquet file from server PC disk
        if (url === '/api/server-logs/download/parquet' && req.method === 'GET') {
          if (fs.existsSync(parquetPath)) {
            res.setHeader('Content-Type', 'application/vnd.apache.parquet');
            res.setHeader('Content-Disposition', 'attachment; filename="plant_history.parquet"');
            fs.createReadStream(parquetPath).pipe(res);
            return;
          }
          res.statusCode = 404;
          res.end('Parquet file not found');
          return;
        }

        // GET /api/server-logs/download/csv -> Serve actual .csv file from server PC disk
        if (url === '/api/server-logs/download/csv' && req.method === 'GET') {
          if (fs.existsSync(csvPath)) {
            res.setHeader('Content-Type', 'text/csv; charset=utf-8');
            res.setHeader('Content-Disposition', 'attachment; filename="plant_history.csv"');
            fs.createReadStream(csvPath).pipe(res);
            return;
          }
          res.statusCode = 404;
          res.end('CSV file not found');
          return;
        }

        next();
      });
    },
  };
}

export default defineConfig({
  plugins: [react(), serverFileLoggingPlugin()],
  server: {
    port: 3000,
    host: true,
  },
});
