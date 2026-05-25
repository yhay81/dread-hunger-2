import { createAppServer } from "./server.js";

const port = Number.parseInt(process.env.PORT ?? "8787", 10);
const host = process.env.HOST ?? "127.0.0.1";

const server = createAppServer();
server.listen(port, host, () => {
  console.log(JSON.stringify({ event: "backend_started", host, port }));
});
