// Relative URLs on purpose: the page lives under Home Assistant's ingress path
// (/api/hassio_ingress/<token>/) and on http://127.0.0.1:8099 in SCREEN_DEV; `api/...`
// resolves against the page in both. Writes carry the CSRF token the inventory handed out.
let csrf = "";
export function setCsrf(token: string) {
  csrf = token || "";
}

export async function api(path: string, options: RequestInit = {}): Promise<Response> {
  const response = await fetch(`api/${path}`, {
    ...options,
    headers: {
      "Content-Type": "application/json",
      "X-Screen-CSRF": csrf,
      ...(options.headers as Record<string, string> | undefined),
    },
  });
  if (!response.ok) {
    let message = "That didn't work. Refresh the page and try again.";
    try {
      message = (await response.json()).error || message;
    } catch {}
    throw new Error(message);
  }
  return response;
}

export const getJson = async <T = any>(path: string): Promise<T> => (await api(path)).json();
export const send = async <T = any>(path: string, method: string, body?: unknown, extra: RequestInit = {}): Promise<T> => {
  const response = await api(path, { method, body: body === undefined ? undefined : JSON.stringify(body), ...extra });
  const text = await response.text();
  return (text ? JSON.parse(text) : null) as T;
};
