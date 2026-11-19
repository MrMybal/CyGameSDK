"""Offline API fixture, not an AI model or authentication test."""
import json,time,threading
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
class ProviderFixture:
    def __init__(self):
        self.requests=[];self.lock=threading.Lock();owner=self
        class Handler(BaseHTTPRequestHandler):
            protocol_version="HTTP/1.1"
            def log_message(self,*args):pass
            def handle(self):
                try:super().handle()
                except (ConnectionResetError,BrokenPipeError):pass
            def do_GET(self):
                data=json.dumps({"data":[{"id":"cygame-fixture","object":"model"}]}).encode()
                self.send_response(200);self.send_header("Content-Type","application/json");self.send_header("Content-Length",str(len(data)));self.end_headers();self.wfile.write(data)
            def do_POST(self):
                request=json.loads(self.rfile.read(int(self.headers["Content-Length"])))
                messages=request["messages"];last=max(i for i,m in enumerate(messages) if m["role"]=="user");text=messages[last]["content"]
                if isinstance(text,list):text=" ".join(m.get("text","") for m in text)
                tools=[m for m in messages[last+1:] if m["role"]=="tool"]
                with owner.lock:owner.requests.append({"attachmentObserved":"ATTACHMENT_FIXTURE" in json.dumps(messages),"messageCount":len(messages),"bytes":len(json.dumps(messages).encode()),"toolResults":len(tools),"toolGroupsValid":all(any(t.get("id")==m.get("tool_call_id") for prev in messages[:i] for t in prev.get("tool_calls",[])) for i,m in enumerate(messages) if m["role"]=="tool")})
                self.send_response(200);self.send_header("Content-Type","text/event-stream");self.send_header("Transfer-Encoding","chunked");self.end_headers()
                def event(delta,finish=None):
                    data=("data: "+json.dumps({"choices":[{"index":0,"delta":delta,"finish_reason":finish}]})+"\n\n").encode()
                    self.wfile.write(("%x\r\n"%len(data)).encode()+data+b"\r\n");self.wfile.flush()
                try:
                    if text.startswith("TOOL_") and len(tools)<3:
                        phase=len(tools)
                        target="property_set" if text in ("TOOL_EDIT","TOOL_DENY") else "scene_list"
                        name=("cy_tools_search","cy_tools_describe","cy_tools_call")[phase]
                        args=({"query":target},{"name":target},{"name":target,"arguments":{"entityId":"hero","propertyId":"name","value":"Assistant fixture edit" if text=="TOOL_EDIT" else "SHOULD_NOT_WRITE"} if target=="property_set" else {}})[phase]
                        event({"tool_calls":[{"index":0,"id":"fixture-call-"+str(phase),"type":"function","function":{"name":name,"arguments":json.dumps(args)}}]});event({},"tool_calls")
                    elif text=="CANCEL_SILENT":
                        event({"content":"Partial response before cancellation"});time.sleep(5);event({"content":" SHOULD_NOT_COMPLETE"});event({},"stop")
                    else:
                        reply="REFUSED" if text=="TOOL_DENY" and any(code in str(tools[-1:]) for code in ("PermissionDenied","AccessDenied")) else "EDIT_DONE" if text=="TOOL_EDIT" else "READ_DONE" if text=="TOOL_READ" else "LONG_REPLY "+("Context sample. "*550) if text.startswith("LONG_") else "FIXTURE_OK"
                        event({"content":reply[:20]});time.sleep(.12);event({"content":reply[20:]});event({},"stop")
                    data=b"data: [DONE]\n\n";self.wfile.write(("%x\r\n"%len(data)).encode()+data+b"\r\n0\r\n\r\n");self.wfile.flush()
                except (BrokenPipeError,ConnectionResetError,OSError):pass
        self.server=ThreadingHTTPServer(("127.0.0.1",0),Handler);self.thread=threading.Thread(target=self.server.serve_forever,daemon=True)
        self.url="http://127.0.0.1:"+str(self.server.server_port)+"/v1"
    def __enter__(self):self.thread.start();return self
    def __exit__(self,*args):self.server.shutdown();self.server.server_close();self.thread.join()
