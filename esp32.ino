#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>


const char* ssid = "network";
const char* password = "passwd";


WiFiUDP udp;
WebServer server(80);
WebSocketsServer ws(81);

StaticJsonDocument<4096> data;

// ================= HTML =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<style>

body{
  margin:0;
  background:#0a0d0f;
  color:#e5e7eb;
  font-family: Verdana;
  overflow-x:hidden;
}

.header{
  padding:16px;
  text-align:center;
  font-size:20px;
  font-weight:bold;
  text-shadow:0 0 10px #60a5fa;
}

h3{
  text-align:center;
  font-size:11px;
  font-weight:normal;
  color:#9ca3af;
  margin:3px 0;
}

.grid{
  display:grid;
  grid-template-columns:repeat(2,1fr);
  gap:10px;
  padding:12px;
}

.card{
  background: rgba(17, 24, 39, 0.75);
  backdrop-filter: blur(8px);
  padding:14px;
  border-radius:12px;
  box-shadow:0 0 10px rgba(96,165,250,0.12);
}

.title{font-size:12px;color:#9ca3af;}
.value{font-size:22px;font-weight:bold;}

.storage{grid-column: span 2;text-align:center;}

#services{
  font-size:12px;
  text-align:center;
  padding:10px 5px;
  white-space:nowrap;
}

.up{color:#22c55e;text-shadow:0 0 10px #22c55e;}
.down{color:#ef4444;text-shadow:0 0 6px #ef4444;}

#chartBox{
  padding:10px;
  width:100%;
  height:320px;
  overflow:hidden;
}

canvas{
  width:100% !important;
  height:100% !important;
  display:block;
  background: rgba(15, 23, 42, 0.75);
  border-radius:14px;
}

#extras{
  text-align:center;
  font-size:12px;
  color:#9ca3af;
  padding:6px;
}

footer{
  margin-top:18px;
  padding:18px;
  text-align:center;
  font-size:11px;
  color:#9ca3af;
}

footer a{
  position: relative;
  display: inline-block;
  color: #e5e7eb;
  text-decoration: none;
  margin: 0 8px;
}

footer a::after{
  content:"";
  position:absolute;
  left:0;
  bottom:0;
  width:100%;
  height:2px;
  background: linear-gradient(90deg, transparent, #60a5fa, transparent);
  transform: scaleX(0);
  transition: transform 0.35s ease;
  filter: drop-shadow(0 0 6px #60a5fa);
}

footer a:hover::after{
  transform: scaleX(1);
}

</style>
</head>

<body>

<div class="header">your text here</div>

<h3>your text here</h3>
<h3>your text here</h3>

<div class="grid">

<div class="card"><div class="title">cpu</div><div class="value" id="cpu">0 %</div></div>
<div class="card"><div class="title">temperature</div><div class="value" id="temp">0 °C</div></div>
<div class="card"><div class="title">frequency</div><div class="value" id="freq">0 GHz</div></div>
<div class="card"><div class="title">ram</div><div class="value" id="ram">0 / 0 gb</div></div>

<div class="card storage">
  <div class="title">storage</div>
  <div class="value" id="disk">0 / 0 gb</div>
</div>

</div>

<div id="chartBox">
  <canvas id="chart"></canvas>
</div>

<div id="services"></div>
<div id="extras"></div>

<script>

const ctx=document.getElementById("chart");

const chart=new Chart(ctx,{
  type:"line",
  data:{labels:[],datasets:[
    {label:"CPU",data:[],borderColor:"#60a5fa",tension:0.4,pointRadius:0},
    {label:"TEMP",data:[],borderColor:"#f87171",tension:0.4,pointRadius:0}
  ]},

  options:{
    animation:false,
    responsive:true,
    maintainAspectRatio:false,

    interaction:{
      mode:"index",
      intersect:false
    },

    plugins:{
      legend:{display:false},

      tooltip:{
        enabled:true,
        mode:"index",
        intersect:false,

        backgroundColor:"rgba(15, 23, 42, 0.95)",
        titleColor:"#e5e7eb",
        bodyColor:"#e5e7eb",

        borderColor:"#60a5fa",
        borderWidth:1,
        padding:10,

        displayColors:false,

        callbacks:{
          title:(items)=>"time: "+items[0].label,
          label:(ctx)=>{
            let v=ctx.parsed.y;
            return ctx.datasetIndex===0
              ? "CPU: "+v.toFixed(1)+" %"
              : "TEMP: "+v.toFixed(1)+" °C";
          }
        }
      }
    },

    scales:{
      x:{display:false},
      y:{grid:{color:"rgba(255,255,255,0.05)"}}
    }
  }
});

let cpuS=null,tempS=null;
const a=0.15;

function updateUI(d){

  const cpu=Number(d.cpu)||0;
  const temp=Number(d.temp)||0;

  cpuS=cpuS??cpu; cpuS+=a*(cpu-cpuS);
  tempS=tempS??temp; tempS+=a*(temp-tempS);

  document.getElementById("cpu").textContent=cpuS.toFixed(1)+" %";
  document.getElementById("temp").textContent=tempS.toFixed(1)+" °C";

  const f=Number(d.cpu_freq)||0;
  document.getElementById("freq").textContent=f?(f/1000).toFixed(2)+" GHz":"0";

  document.getElementById("ram").textContent =
    (Number(d.ram_used)||0).toFixed(2) + " GB / " +
    (Number(d.ram_total)||0).toFixed(2) + " GB";
  document.getElementById("disk").textContent =
    (Number(d.disk_used)||0).toFixed(2) + " GB / " +
    (Number(d.disk_total)||0).toFixed(2) + " GB";

  const s=d.services||{};

  document.getElementById("services").innerHTML=
    "jellyfin "+(s.jellyfin?'<span class="up">up</span>':'<span class="down">down</span>')+" | "+
    "radarr "+(s.radarr?'<span class="up">up</span>':'<span class="down">down</span>')+" | "+
    "sonarr "+(s.sonarr?'<span class="up">up</span>':'<span class="down">down</span>');

  document.getElementById("extras").innerHTML=
    "uptime "+Math.floor((d.uptime??0)/60)+" min | "+

  const now = new Date();

const t =
  String(now.getHours()).padStart(2,'0') + ':' +
  String(now.getMinutes()).padStart(2,'0');

  chart.data.labels.push(t);
  chart.data.datasets[0].data.push(cpuS);
  chart.data.datasets[1].data.push(tempS);

  if(chart.data.labels.length>40){
    chart.data.labels.shift();
    chart.data.datasets.forEach(x=>x.data.shift());
  }

  chart.update('none');
}

setInterval(async()=>{
  try{
    const r=await fetch("/data?x="+Date.now());
    updateUI(await r.json());
  }catch(e){}
},2000);

</script>

<footer>
  <div>contact</div><br>
  <a href="https://discord.com/">discord</a>
  <a href="https://github.com/">github</a>
</footer>

</body>
</html>
)rawliteral";

// ================= SERVER =================
void handleRoot(){ server.send_P(200,"text/html",index_html); }

void handleData(){
  String out;
  serializeJson(data,out);
  server.send(200,"application/json",out);
}

void wsEvent(uint8_t, WStype_t type, uint8_t*, size_t){
  if(type==WStype_CONNECTED) Serial.println("WS connected");
}

void handleUDP(){
  int p=udp.parsePacket();
  if(p){
    char b[4096];
    int l=udp.read(b,sizeof(b)-1);
    if(l>0){
      b[l]=0;
      if(!deserializeJson(data,b)){
        String j; serializeJson(data,j);
        ws.broadcastTXT(j);
      }
    }
  }
}

void setup(){
  Serial.begin(115200);

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(300);
    Serial.print(".");
  }

  udp.begin(4210);

  server.on("/",handleRoot);
  server.on("/data",handleData);
  server.begin();

  ws.begin();
  ws.onEvent(wsEvent);
}

void loop(){
  server.handleClient();
  ws.loop();
  handleUDP();
}
