import http from 'http';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const PORT = process.env.PORT || 3001;
const dataDir = path.resolve(__dirname, 'data');
const jsonPath = path.join(dataDir, 'plant_history.json');
const csvPath = path.join(dataDir, 'plant_history.csv');
const parquetPath = path.join(dataDir, 'plant_history.parquet');

if (!fs.existsSync(dataDir)) {
  fs.mkdirSync(dataDir, { recursive: true });
}

function loadRecords() {
  try {
    if (fs.existsSync(jsonPath)) {
      return JSON.parse(fs.readFileSync(jsonPath, 'utf-8'));
    }
  } catch (e) {
    console.error('Error loading records:', e);
  }
  return [];
}

function saveRecordsToFiles(records) {
  try {
    fs.writeFileSync(jsonPath, JSON.stringify(records, null, 2), 'utf-8');

    const headers = ['Timestamp', 'DateTime_JST', 'Seq', 'Stress', 'Status', 'SoilTrend', 'AirTemp', 'Humidity', 'LeafTemp', 'DeltaT', 'SoilRaw', 'Lux', 'TankLiquid', 'PumpOn', 'AiTrainCount', 'AiLoss', 'AiPhase', 'AiAnomalyScore'];
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
    fs.writeFileSync(parquetPath, Buffer.concat([magic, lenBuf, payload, magic]));
    console.log(`[Server PC Log] Saved ${records.length} records to ${dataDir}`);
  } catch (e) {
    console.error('Error writing files:', e);
  }
}

const server = http.createServer((req, res) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  const url = req.url || '';

  if (url === '/api/server-logs' && req.method === 'GET') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(loadRecords()));
    return;
  }

  if (url === '/api/server-logs' && req.method === 'POST') {
    let body = '';
    req.on('data', (c) => (body += c));
    req.on('end', () => {
      try {
        const record = JSON.parse(body);
        const records = loadRecords();
        if (!records.some((r) => r.timestamp === record.timestamp)) {
          records.push(record);
          saveRecordsToFiles(records);
        }
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'ok', count: records.length, saved_to: dataDir }));
      } catch (e) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ error: 'Invalid JSON' }));
      }
    });
    return;
  }

  if (url === '/api/server-logs/clear' && req.method === 'POST') {
    saveRecordsToFiles([]);
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'cleared', saved_to: dataDir }));
    return;
  }

  if (url === '/api/server-logs/download/parquet') {
    if (fs.existsSync(parquetPath)) {
      res.writeHead(200, {
        'Content-Type': 'application/vnd.apache.parquet',
        'Content-Disposition': 'attachment; filename="plant_history.parquet"',
      });
      fs.createReadStream(parquetPath).pipe(res);
      return;
    }
  }

  if (url === '/api/server-logs/download/csv') {
    if (fs.existsSync(csvPath)) {
      res.writeHead(200, {
        'Content-Type': 'text/csv; charset=utf-8',
        'Content-Disposition': 'attachment; filename="plant_history.csv"',
      });
      fs.createReadStream(csvPath).pipe(res);
      return;
    }
  }

  res.writeHead(404);
  res.end('Not Found');
});

server.listen(PORT, () => {
  console.log(`Plant Medical Storage Server running at http://localhost:${PORT}`);
  console.log(`Logs storing directly on Server PC at: ${dataDir}`);
});
