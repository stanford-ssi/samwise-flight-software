import React, { useState } from 'react';
import { LogData, TaskInfo } from './types';
import { Timeline } from './components/Timeline';
import { EventLog } from './components/EventLog';
import { TaskList } from './components/TaskList';

const TASK_COLORS = [
  '#ef4444', '#f59e0b', '#10b981', '#3b82f6', '#8b5cf6',
  '#ec4899', '#14b8a6', '#f97316', '#06b6d4', '#84cc16'
];

function App() {
  const [logData, setLogData] = useState<LogData | null>(null);
  const [tasks, setTasks] = useState<TaskInfo[]>([]);
  const [selectedTasks, setSelectedTasks] = useState<Set<string>>(new Set());

  const handleFileUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      try {
        const data = JSON.parse(e.target?.result as string) as LogData;
        setLogData(data);

        // Extract task information from task_discovered events
        const discoveredTasks = new Map<string, TaskInfo>();
        data.events.forEach((event) => {
          if (event.event === 'task_discovered' && event.task) {
            const periodMatch = event.details.match(/period=(\d+)/);
            const indexMatch = event.details.match(/index=(\d+)/);

            if (periodMatch && indexMatch) {
              const index = parseInt(indexMatch[1]);
              discoveredTasks.set(event.task, {
                name: event.task,
                period_ms: parseInt(periodMatch[1]),
                index: index,
                color: TASK_COLORS[index % TASK_COLORS.length],
              });
            }
          }
        });

        const taskList = Array.from(discoveredTasks.values()).sort(
          (a, b) => a.index - b.index
        );
        setTasks(taskList);

        // Initially select all tasks
        setSelectedTasks(new Set(taskList.map(t => t.name)));
      } catch (error) {
        console.error('Error parsing log file:', error);
        alert('Error parsing log file. Please ensure it is valid JSON.');
      }
    };
    reader.readAsText(file);
  };

  return (
    <div style={{
      minHeight: '100vh',
      backgroundColor: '#0a0a0a',
      color: '#fff',
      fontFamily: 'system-ui, -apple-system, sans-serif'
    }}>
      <header style={{
        padding: '20px',
        backgroundColor: '#111',
        borderBottom: '2px solid #333',
      }}>
        <h1 style={{ margin: 0, fontSize: '24px' }}>
          Satellite Running State Visualizer
        </h1>
        <p style={{ margin: '5px 0 0 0', color: '#888', fontSize: '14px' }}>
          Upload the running_state_viz.json file to visualize task execution
        </p>
      </header>

      <div style={{ padding: '20px' }}>
        <div style={{
          padding: '20px',
          backgroundColor: '#1a1a1a',
          borderRadius: '8px',
          marginBottom: '20px',
        }}>
          <label htmlFor="file-upload" style={{
            display: 'inline-block',
            padding: '10px 20px',
            backgroundColor: '#3b82f6',
            color: '#fff',
            borderRadius: '6px',
            cursor: 'pointer',
            fontSize: '14px',
            fontWeight: '500',
          }}>
            Load Log File
          </label>
          <input
            id="file-upload"
            type="file"
            accept=".json"
            onChange={handleFileUpload}
            style={{ display: 'none' }}
          />
        </div>

        {logData ? (
          <>
            <TaskList
              tasks={tasks}
              selectedTasks={selectedTasks}
              onToggleTask={(taskName) => {
                const newSelected = new Set(selectedTasks);
                if (newSelected.has(taskName)) {
                  newSelected.delete(taskName);
                } else {
                  newSelected.add(taskName);
                }
                setSelectedTasks(newSelected);
              }}
            />
            <Timeline events={logData.events} tasks={tasks} selectedTasks={selectedTasks} />
            <EventLog events={logData.events} selectedTasks={selectedTasks} />
          </>
        ) : (
          <div style={{
            textAlign: 'center',
            padding: '60px 20px',
            color: '#666',
          }}>
            <div style={{ fontSize: '48px', marginBottom: '20px' }}>📊</div>
            <h2>No data loaded</h2>
            <p>Please upload a visualization log file to begin</p>
          </div>
        )}
      </div>
    </div>
  );
}

export default App;
