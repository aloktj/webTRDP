import { useEffect, useMemo, useState } from 'react';
import './app.css';

type PdTelegram = {
  name?: string;
  com_id?: number;
  dataset_id?: number;
  direction?: string;
  cycle_us?: number;
  interface?: string;
  tx_enabled: boolean;
  next_tx_due_us: number;
  tx_payload_size: number;
  last_rx_payload_size: number;
  last_rx_time_us: number;
  last_rx_valid: boolean;
  rx_count: number;
  tx_count: number;
  timeout_count: number;
  last_period_us: number;
  avg_period_us: number;
};

function formatMicros(micros: number) {
  if (!micros) return '—';
  const ms = micros / 1000;
  return `${ms.toFixed(1)} ms`;
}

function formatDirection(direction?: string) {
  if (!direction) return '—';
  return direction
    .split('_')
    .map((part) => part[0]?.toUpperCase() + part.slice(1))
    .join(' / ');
}

function formatTimeAgo(micros: number) {
  if (!micros) return '—';
  const date = new Date(micros / 1000);
  const diffSeconds = (Date.now() - date.getTime()) / 1000;
  if (diffSeconds < 1) return 'just now';
  if (diffSeconds < 60) return `${Math.floor(diffSeconds)}s ago`;
  const minutes = Math.floor(diffSeconds / 60);
  if (minutes < 60) return `${minutes}m ago`;
  const hours = Math.floor(minutes / 60);
  if (hours < 24) return `${hours}h ago`;
  const days = Math.floor(hours / 24);
  return `${days}d ago`;
}

function App() {
  const [backendUrl, setBackendUrl] = useState('http://localhost:8080');
  const [configPath, setConfigPath] = useState('third_party/TCNopen/trdp/test/xml/example.xml');
  const [hostName, setHostName] = useState('localhost');
  const [pdTelegrams, setPdTelegrams] = useState<PdTelegram[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string>('');
  const [status, setStatus] = useState<string>('');

  const apiBase = useMemo(() => backendUrl.replace(/\/$/, ''), [backendUrl]);

  const fetchTelegrams = async () => {
    setIsLoading(true);
    setError('');
    setStatus('');
    try {
      const resp = await fetch(`${apiBase}/api/pd/telegrams`);
      if (!resp.ok) {
        throw new Error(`Request failed with status ${resp.status}`);
      }
      const data: PdTelegram[] = await resp.json();
      setPdTelegrams(data);
      setStatus(`Loaded ${data.length} process data telegrams.`);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load telegrams');
    } finally {
      setIsLoading(false);
    }
  };

  const loadConfig = async () => {
    setIsLoading(true);
    setError('');
    setStatus('');
    try {
      const resp = await fetch(`${apiBase}/api/configs/load`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ path: configPath, host_name: hostName }),
      });

      if (!resp.ok) {
        const message = await resp.text();
        throw new Error(message || `Config load failed (${resp.status})`);
      }

      setStatus('Configuration loaded. Refresh telegrams to see updates.');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load configuration');
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchTelegrams().catch(() => setError('Unable to reach backend. Check the URL and try again.'));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  return (
    <main className="app-shell">
      <header className="page-header">
        <div>
          <p className="eyebrow">TRDP Simulator</p>
          <h1>webTRDP Dashboard</h1>
          <p className="lede">
            Monitor process data telegrams, toggle configuration, and verify connectivity with the backend API.
          </p>
        </div>
      </header>

      <section className="card">
        <div className="card-header">
          <div>
            <p className="eyebrow">Connection</p>
            <h2>Backend settings</h2>
          </div>
          <button className="ghost" onClick={fetchTelegrams} disabled={isLoading}>
            {isLoading ? 'Refreshing…' : 'Refresh telegrams'}
          </button>
        </div>

        <div className="field-grid">
          <label className="field">
            <span>Backend URL</span>
            <input
              type="text"
              value={backendUrl}
              onChange={(e) => setBackendUrl(e.target.value)}
              placeholder="http://localhost:8080"
            />
          </label>
          <label className="field">
            <span>Config XML path</span>
            <input
              type="text"
              value={configPath}
              onChange={(e) => setConfigPath(e.target.value)}
              placeholder="third_party/TCNopen/trdp/test/xml/example.xml"
            />
          </label>
          <label className="field">
            <span>Host name</span>
            <input type="text" value={hostName} onChange={(e) => setHostName(e.target.value)} placeholder="localhost" />
          </label>
        </div>

        <div className="actions">
          <button onClick={loadConfig} disabled={isLoading}>
            {isLoading ? 'Loading…' : 'Load configuration'}
          </button>
        </div>

        {(error || status) && (
          <div className={error ? 'banner error' : 'banner success'}>{error ? error : status}</div>
        )}
      </section>

      <section className="card">
        <div className="card-header">
          <div>
            <p className="eyebrow">Process Data</p>
            <h2>PD telegrams</h2>
          </div>
          <p className="muted">Live snapshot from the TRDP backend</p>
        </div>

        <div className="table-wrapper">
          <table>
            <thead>
              <tr>
                <th>Name</th>
                <th>COM ID</th>
                <th>Dataset</th>
                <th>Direction</th>
                <th>Cycle</th>
                <th>Interface</th>
                <th>Tx enabled</th>
                <th>Tx count</th>
                <th>Rx count</th>
                <th>Last Rx</th>
              </tr>
            </thead>
            <tbody>
              {pdTelegrams.length === 0 && (
                <tr>
                  <td colSpan={10} className="muted center">
                    {isLoading ? 'Loading…' : 'No telegrams returned yet. Try refreshing or loading a configuration.'}
                  </td>
                </tr>
              )}
              {pdTelegrams.map((pd, idx) => (
                <tr key={pd.com_id ?? idx}>
                  <td>{pd.name ?? '—'}</td>
                  <td>{pd.com_id ?? '—'}</td>
                  <td>{pd.dataset_id ?? '—'}</td>
                  <td>{formatDirection(pd.direction)}</td>
                  <td>{pd.cycle_us ? formatMicros(pd.cycle_us) : '—'}</td>
                  <td>{pd.interface ?? '—'}</td>
                  <td>
                    <span className={pd.tx_enabled ? 'pill success' : 'pill'}>{pd.tx_enabled ? 'Enabled' : 'Disabled'}</span>
                  </td>
                  <td>{pd.tx_count}</td>
                  <td>{pd.rx_count}</td>
                  <td title={formatMicros(pd.last_period_us)}>{pd.last_rx_valid ? formatTimeAgo(pd.last_rx_time_us) : '—'}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>
    </main>
  );
}

export default App;
