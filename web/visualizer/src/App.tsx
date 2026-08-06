import React, { useState } from 'react';
import { LogData, TaskInfo, StateSpan, StateTransition } from './types';
import { StateMachine } from './components/StateMachine';
import { Timeline } from './components/Timeline';
import { EventLog } from './components/EventLog';
import { TaskList } from './components/TaskList';

const TASK_COLORS = [
  '#ef4444', '#f59e0b', '#10b981', '#3b82f6', '#8b5cf6',
  '#ec4899', '#14b8a6', '#f97316', '#06b6d4', '#84cc16'
];

const STATE_COLORS = [
  '#6366f1', '#22c55e', '#f59e0b', '#ef4444', '#06b6d4',
  '#a855f7', '#ec4899', '#14b8a6',
];

function App() {
  const [logData, setLogData] = useState<LogData | null>(null);
  const [tasks, setTasks] = useState<TaskInfo[]>([]);
  const [selectedTasks, setSelectedTasks] = useState<Set<string>>(new Set());
  const [stateSpans, setStateSpans] = useState<StateSpan[]>([]);
  const [transitions, setTransitions] = useState<StateTransition[]>([]);
  const [profileName, setProfileName] = useState<string>('');

  const handleFileUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (e) => {
      try {
        const data = JSON.parse(e.target?.result as string) as LogData;
        setLogData(data);

        // Extract profile name from fsm_start event
        const fsmStartEvent = data.events.find(ev => ev.event === 'fsm_start');
        if (fsmStartEvent) {
          const profileMatch = fsmStartEvent.details.match(/profile=(.+)/);
          if (profileMatch) {
            setProfileName(profileMatch[1]);
          }
        }

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
        setSelectedTasks(new Set(taskList.map(t => t.name)));

        // Extract state spans and transitions from state_enter/state_exit events
        const spans: StateSpan[] = [];
        const trans: StateTransition[] = [];
        const stateColorMap = new Map<string, string>();
        let colorIdx = 0;

        data.events.forEach((ev) => {
          if (ev.event === 'state_enter') {
            const stateMatch = ev.details.match(/state=([^,]+)/);
            const fromMatch = ev.details.match(/from=([^,]+)/);
            if (stateMatch) {
              const stateName = stateMatch[1];
              if (!stateColorMap.has(stateName)) {
                stateColorMap.set(stateName, STATE_COLORS[colorIdx % STATE_COLORS.length]);
                colorIdx++;
              }

              // Collect tasks discovered for this state (events after this enter)
              const stateTasks: string[] = [];
              const enterIdx = data.events.indexOf(ev);
              for (let i = enterIdx + 1; i < data.events.length; i++) {
                const nextEv = data.events[i];
                if (nextEv.event === 'task_discovered') {
                  stateTasks.push(nextEv.task);
                } else if (nextEv.event === 'state_exit' || nextEv.event === 'state_enter') {
                  break;
                }
              }

              spans.push({
                name: stateName,
                enter_time_ms: ev.time_ms,
                exit_time_ms: null,
                tasks: stateTasks,
                color: stateColorMap.get(stateName)!,
              });

              if (fromMatch && fromMatch[1] !== 'none') {
                trans.push({
                  from_state: fromMatch[1],
                  to_state: stateName,
                  time_ms: ev.time_ms,
                });
              }
            }
          } else if (ev.event === 'state_exit') {
            // Close the most recent open span
            for (let i = spans.length - 1; i >= 0; i--) {
              if (spans[i].exit_time_ms === null) {
                spans[i].exit_time_ms = ev.time_ms;
                break;
              }
            }
          }
        });

        setStateSpans(spans);
        setTransitions(trans);
      } catch (error) {
        console.error('Error parsing log file:', error);
        alert('Error parsing log file. Please ensure it is valid JSON.');
      }
    };
    reader.readAsText(file);
  };

  const hasFsmData = stateSpans.length > 0;
  const maxTime = logData
    ? Math.max(...logData.events.map(e => e.time_ms), 1000)
    : 1000;

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
          SAMWISE FSM Visualizer
        </h1>
        <p style={{ margin: '5px 0 0 0', color: '#888', fontSize: '14px' }}>
          Upload a visualization JSON file to visualize state transitions and task execution
          {profileName && (
            <span style={{ color: '#6366f1', marginLeft: '10px', fontWeight: 500 }}>
              [{profileName}]
            </span>
          )}
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
            {hasFsmData && (
              <StateMachine
                stateSpans={stateSpans}
                transitions={transitions}
                maxTime={maxTime}
              />
            )}
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
            <Timeline
              events={logData.events}
              tasks={tasks}
              selectedTasks={selectedTasks}
              stateSpans={hasFsmData ? stateSpans : undefined}
            />
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
