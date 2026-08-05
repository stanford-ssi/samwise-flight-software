export interface LogEvent {
  time_ms: number;
  event: string;
  task: string;
  details: string;
}

export interface LogData {
  events: LogEvent[];
}

export interface TaskInfo {
  name: string;
  period_ms: number;
  index: number;
  color: string;
}

export interface StateSpan {
  name: string;
  enter_time_ms: number;
  exit_time_ms: number | null; // null if still active at end
  tasks: string[];
  color: string;
}

export interface StateTransition {
  from_state: string;
  to_state: string;
  time_ms: number;
}
