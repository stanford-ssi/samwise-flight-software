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
