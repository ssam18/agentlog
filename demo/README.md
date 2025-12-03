# AgentLog Demo Environment

Complete end-to-end demonstration of AgentLog with simulated Jira, PagerDuty, Slack services, and a realistic payment processing application.

## 🎯 Overview

This demo environment provides:

- **🎫 Mock Jira Server** - Simulates Jira REST API for ticket creation
- **🚨 Mock PagerDuty Server** - Simulates PagerDuty Events API v2 for incident management
- **💬 Mock Slack Server** - Simulates Slack Incoming Webhooks for notifications
- **💾 RocksDB Container** - Persistent storage (ready for integration)
- **📊 Web Dashboard** - Real-time monitoring of all events
- **💳 Payment Service Demo** - Realistic C++ application using AgentLog

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│              Payment Service Demo (C++ Application)             │
│                   Uses AgentLog Library                         │
│                                                                 │
└────────────┬────────────────────────────────────┬───────────────┘
             │                                    │
             │ Logs, Metrics, Anomalies          │ Webhooks
             │                                    │
             ▼                                    ▼
┌────────────────────────┐         ┌─────────────────────────────┐
│                        │         │    Integration Layer        │
│   AgentLog Engine      │────────▶│  • Jira Tickets            │
│  • Anomaly Detection   │         │  • PagerDuty Alerts        │
│  • Pattern Recognition │         │  • Slack Notifications     │
│  • Event Correlation   │         │  • RocksDB Storage         │
│                        │         │                             │
└────────────────────────┘         └──────────┬──────────────────┘
                                              │
                                              ▼
                     ┌────────────────────────────────────────┐
                     │       Mock Services (Docker)           │
                     │  • mock-jira:8080                      │
                     │  • mock-pagerduty:8081                 │
                     │  • mock-slack:8082                     │
                     │  • dashboard:3000                      │
                     └────────────────────────────────────────┘
```

## 🚀 Quick Start

### Prerequisites

- Docker and Docker Compose (or Podman with Docker CLI)
- CMake 3.15+
- C++17 compiler (GCC 7+ or Clang 6+)
- libcurl development libraries
- Web browser (Chrome, Firefox, Safari, etc.)

### 🎬 Live Interactive Demo (Recommended)

Run the complete demo with automatic browser opening and real-time transaction processing:

```bash
cd demo
./live-demo.sh 60  # Run for 60 seconds (default: 30)
```

**What happens:**
1. ✅ Starts all Docker services (Jira, PagerDuty, Slack, Dashboard)
2. ✅ Waits for health checks to pass
3. ✅ Builds the C++ demo application
4. ✅ Opens web UIs in your browser automatically
5. ✅ Runs payment demo generating live transactions
6. ✅ Shows real-time statistics and summary

### 📺 Observe the Demo in Action

After running `./live-demo.sh`, **open these URLs in your browser** (they auto-refresh every 2 seconds):

| URL | What You'll See | Purpose |
|-----|-----------------|---------|
| **http://localhost:8080** | 🎫 **Jira Tickets** | View all fraud detection tickets in a beautiful table with priorities, labels, and descriptions |
| **http://localhost:8081** | 🚨 **PagerDuty Incidents** | See all high-latency and timeout incidents with severity levels and status tracking |
| **http://localhost:8082** | 💬 **Slack Messages** | Browse all notification messages with attachments and formatting |
| **http://localhost:3000** | 📊 **Dashboard** | Monitor aggregated stats across all services in real-time |

**💡 Pro Tip:** Arrange all 4 URLs in different browser windows/tabs to watch events flow in real-time!

### 🎯 Manual Step-by-Step (For Development)

If you want to control each step individually:

```bash
# 1. Start all services
cd demo
docker compose up -d

# 2. Wait for services to be ready (check health)
curl http://localhost:8080/health
curl http://localhost:8081/health
curl http://localhost:8082/health

# 3. Build the demo application
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)

# 4. Open web UIs in your browser:
#    - http://localhost:8080  (Jira)
#    - http://localhost:8081  (PagerDuty)
#    - http://localhost:8082  (Slack)
#    - http://localhost:3000  (Dashboard)

# 5. Run the demo application
./simple_payment_demo
# Press Ctrl+C to stop when done

# 6. Stop all services
cd ..
docker compose down
```

## 📊 Monitoring and Observing Results

### 🌐 Web User Interfaces

All services provide beautiful, auto-refreshing web UIs (updates every 2 seconds):

#### 🎫 Jira Web UI - http://localhost:8080

**What you'll see:**
- Table view of all tickets (like real Jira)
- Ticket keys (AGENT-1000, AGENT-1001, etc.)
- Priority badges with colors (🔴 Critical, 🟡 High, 🔵 Medium, 🟢 Low)
- Issue types with icons (🐛 Bug, 📋 Task, 📖 Story)
- Labels (fraud-detection, payment-service, agentlog)
- Creation timestamps (Just now, 2m ago, 1h ago, etc.)
- Full descriptions with transaction details

**Triggered by:** Fraud detection, critical errors, repeated failures

#### 🚨 PagerDuty Web UI - http://localhost:8081

**What you'll see:**
- Incident cards with color-coded severity
- Status badges (🔴 Triggered, 🟡 Acknowledged, 🟢 Resolved)
- Dedup keys for tracking (payment-latency-TXN-12345)
- Custom details (transaction_id, latency_ms, threshold_ms)
- Real-time statistics (X triggered, Y acked, Z resolved)
- Source information (payment-service, payment-gateway)

**Triggered by:** High latency (>2000ms), timeouts, service unavailability

#### 💬 Slack Web UI - http://localhost:8082

**What you'll see:**
- Message feed in channel format (#agentlog-alerts)
- Bot messages with emoji and formatting
- Color-coded attachments (🔴 Danger, 🟡 Warning, 🟢 Good)
- Field details (Service, Severity, Transaction ID)
- Message timestamps
- Rich formatting with sections and dividers

**Triggered by:** Warnings, errors, anomaly detections, insufficient funds

#### 📊 Dashboard Web UI - http://localhost:3000

**What you'll see:**
- Three stat cards with live counts
- Blue card: Jira Tickets count
- Red card: PagerDuty Incidents count  
- Purple card: Slack Messages count
- Clear buttons to reset each service
- Gradient purple background
- Auto-refresh every 2 seconds

**Purpose:** Aggregated real-time view of all events across services

### 🖥️ Console Output & Logs

#### View Live Demo Output

Watch the C++ application console for real-time transaction processing:

```bash
cd demo/build
./simple_payment_demo
```

**Sample Output:**
```
[1] TXN-433676 - $58.99 - ✓ SUCCESS (latency: 300ms)
[2] TXN-291123 - $34.74 - ✓ SUCCESS (latency: 430ms)
[3] TXN-235422 - $457.75 - ✓ SUCCESS (latency: 284ms)
[4] TXN-605660 - $422.57 - ⚠️  INSUFFICIENT FUNDS
  💬 Slack notification sent
[7] TXN-407613 - $260.91 - 🚫 FRAUD DETECTED - Jira ticket created
  🎫 Jira ticket created for TXN-407613
  🚨 PagerDuty alert sent for TXN-407613-fraud
  💬 Slack notification sent

📊 SUMMARY (after 20 transactions):
   ✅ Success: 16 (80%)
   🚫 Fraud: 1 (5%)
   🔴 Timeouts: 1 (5%)
   ⚠️  Other Errors: 2 (10%)
```

#### View Simulator Logs

Watch colorful console output from each simulator:

```bash
# View Jira simulator logs (shows ticket creation)
docker logs agentlog-mock-jira -f

# View PagerDuty simulator logs (shows incidents)
docker logs agentlog-mock-pagerduty -f

# View Slack simulator logs (shows messages)
docker logs agentlog-mock-slack -f

# View all logs together
docker compose logs -f
```

#### Export Logs to Files

Save all logs for analysis:

```bash
cd demo

# Export individual simulator logs
docker logs agentlog-mock-jira > jira_simulator.log 2>&1
docker logs agentlog-mock-pagerduty > pagerduty_simulator.log 2>&1
docker logs agentlog-mock-slack > slack_simulator.log 2>&1

# Export combined logs
docker compose logs > all_services.log 2>&1

# View exported logs
cat jira_simulator.log
less pagerduty_simulator.log
grep "FRAUD" all_services.log
```

### 🔍 Testing Individual Services

Test each service independently using curl:

```bash
# Create a test Jira ticket
curl -X POST http://localhost:8080/rest/api/2/issue \
  -H "Content-Type: application/json" \
  -d '{
    "fields": {
      "project": {"key": "AGENT"},
      "summary": "Test fraud ticket",
      "description": "Testing Jira simulator",
      "issuetype": {"name": "Bug"},
      "priority": {"name": "Critical"},
      "labels": ["test", "fraud"]
    }
  }'

# Create a PagerDuty incident
curl -X POST http://localhost:8081/v2/enqueue \
  -H "Content-Type: application/json" \
  -d '{
    "routing_key": "test",
    "event": {
      "event_action": "trigger",
      "dedup_key": "test-incident",
      "payload": {
        "summary": "Test high latency",
        "severity": "critical",
        "source": "payment-service"
      }
    }
  }'

# Send a Slack message
curl -X POST http://localhost:8082/services/T/B/test \
  -H "Content-Type: application/json" \
  -d '{
    "text": "⚠️ Test alert",
    "channel": "#alerts"
  }'

# Check service health
curl http://localhost:8080/health
curl http://localhost:8081/health
curl http://localhost:8082/health
```

**Then refresh the web UIs to see your test data appear!**

### Service Endpoints (API Reference)

| Service | URL | Description |
|---------|-----|-------------|
| **Dashboard Web UI** | http://localhost:3000 | Aggregated monitoring dashboard |
| **Dashboard API** | http://localhost:3000/api/stats | JSON stats from all services |
| **Jira Web UI** | http://localhost:8080 | View all tickets in table format |
| **Jira API** | http://localhost:8080/rest/api/2/issue | Create tickets (POST) |
| **Jira Tickets** | http://localhost:8080/tickets | Get all tickets (GET) |
| **PagerDuty Web UI** | http://localhost:8081 | View all incidents with status |
| **PagerDuty API** | http://localhost:8081/v2/enqueue | Trigger incidents (POST) |
| **PagerDuty Incidents** | http://localhost:8081/incidents | Get all incidents (GET) |
| **Slack Web UI** | http://localhost:8082 | View all messages in feed |
| **Slack Webhook** | http://localhost:8082/services/\*/incoming-webhook | Send messages (POST) |
| **Slack Messages** | http://localhost:8082/messages | Get all messages (GET) |

### Console Output

Each simulator displays colorful, formatted output in its console:

#### Jira Simulator Output
```
🎫 NEW TICKET CREATED
────────────────────────────────────────────────────────────────────
Ticket ID:   AGENT-1001
Priority:    🔴 High
Type:        Bug
Summary:     Payment gateway timeout detected
Description: Timeout occurred while processing transaction TXN-543210
Labels:      agentlog, auto-created, payment-service
Created:     2025-12-02 10:15:30
────────────────────────────────────────────────────────────────────
✓ Total tickets created: 5
```

#### PagerDuty Simulator Output
```
🚨 INCIDENT TRIGGERED
────────────────────────────────────────────────────────────────────
Dedup Key:   payment-latency-spike-001
Severity:    🔴 CRITICAL
Summary:     High latency detected in payment processing
Source:      payment-service
Timestamp:   2025-12-02T10:15:30Z
Component:   payment-gateway
────────────────────────────────────────────────────────────────────
■ Triggered: 3  ■ Acknowledged: 1  ■ Resolved: 2
```

#### Slack Simulator Output
```
💬 NEW SLACK MESSAGE
────────────────────────────────────────────────────────────────────
Channel:     #agentlog-alerts
From:        AgentLog Bot
Icon:        :robot_face:
Message:     ⚠️ Anomaly detected in payment processing

Attachments:
  🟡 Attachment 1
  📌 Payment Anomaly Alert
  Anomaly score: 0.95
  Transaction ID: TXN-543210
  Latency: 2500ms (expected < 500ms)
  
  Fields:
    • Service: payment-service
    • Environment: production
    • Threshold: 0.85
────────────────────────────────────────────────────────────────────
✓ Total messages received: 12
```

## 💳 Payment Service Demo

The demo simulates a realistic e-commerce payment processing service.

### Simulated Scenarios

| Scenario | Frequency | Triggers |
|----------|-----------|----------|
| **Successful Payment** | 70% | Normal logging |
| **Insufficient Funds** | 15% | Warning log |
| **Gateway Timeout** | 10% | ERROR log + PagerDuty alert + Anomaly detection |
| **Fraud Detection** | 5% | ERROR log + Jira ticket creation |
| **Database Error** | 2% | ERROR log + Slack notification |
| **API Failure** | 10% | ERROR log + Metric spike |

### Events Generated

1. **Payment Processing**
   - Transaction IDs: `TXN-######`
   - Amount: $10 - $500
   - Payment methods: credit_card, debit_card, paypal, stripe, apple_pay
   - Customer IDs: CUST-001 through CUST-005

2. **Anomaly Detection**
   - Triggered on latency > 2000ms
   - Triggers PagerDuty incident
   - Anomaly score > 0.85

3. **Pattern Recognition**
   - Detects repeated failures
   - Creates Jira tickets for investigation

4. **Integration Triggers**
   - **Jira**: Fraud detection, critical errors
   - **PagerDuty**: High latency, service down
   - **Slack**: Warnings, errors, anomalies

## 🛠️ Configuration

### config.json

The demo uses `config.json` for configuration:

```json
{
  "agentlog": {
    "service_name": "payment-service",
    "anomaly_detection": {
      "enabled": true,
      "threshold": 0.8
    },
    "integrations": {
      "jira": {
        "enabled": true,
        "url": "http://mock-jira:8080",
        "project_key": "AGENT"
      },
      "pagerduty": {
        "enabled": true,
        "url": "http://mock-pagerduty:8081/v2/enqueue"
      },
      "slack": {
        "enabled": true,
        "webhook_url": "http://mock-slack:8082/services/..."
      }
    }
  }
}
```

### Customization

#### Change Service Ports

Edit `docker-compose.yml`:

```yaml
services:
  mock-jira:
    ports:
      - "9090:8080"  # Change from 8080 to 9090
```

#### Adjust Thresholds

Edit `config.json`:

```json
{
  "thresholds": {
    "anomaly_score": 0.90,        # Increase to trigger less often
    "latency_p99_ms": 2000,       # Latency threshold
    "consecutive_failures": 5     # Number of failures before alert
  }
}
```

#### Modify Demo Scenarios

Edit `payment_demo.cpp` and adjust probabilities:

```cpp
if (outcome <= 70) {
    // 70% success - change to 80 for more success
    logger.info("payment.success", ...);
}
```

## 📝 Available Commands

### run-demo.sh Commands

```bash
./run-demo.sh start     # Build and run complete demo
./run-demo.sh build     # Build AgentLog and demo app
./run-demo.sh services  # Start Docker services only
./run-demo.sh demo      # Run demo app (services must be running)
./run-demo.sh logs      # View service logs
./run-demo.sh stop      # Stop all services
./run-demo.sh cleanup   # Stop and remove all data
./run-demo.sh menu      # Interactive menu (default)
```

### Docker Compose Commands

```bash
# View logs from all services
docker-compose logs -f

# View logs from specific service
docker-compose logs -f mock-jira

# Restart a service
docker-compose restart mock-pagerduty

# Stop services
docker-compose down

# Stop and remove volumes
docker-compose down -v
```

## 🧪 Testing Scenarios

### Test Individual Services

#### Test Jira Simulator

```bash
curl -X POST http://localhost:8080/rest/api/2/issue \
  -H "Content-Type: application/json" \
  -d '{
    "fields": {
      "project": {"key": "AGENT"},
      "summary": "Test ticket from curl",
      "description": "This is a test ticket",
      "issuetype": {"name": "Bug"},
      "priority": {"name": "High"}
    }
  }'
```

#### Test PagerDuty Simulator

```bash
curl -X POST http://localhost:8081/v2/enqueue \
  -H "Content-Type: application/json" \
  -d '{
    "routing_key": "test-key",
    "event": {
      "event_action": "trigger",
      "dedup_key": "test-incident-001",
      "payload": {
        "summary": "Test incident from curl",
        "severity": "critical",
        "source": "test",
        "timestamp": "2025-12-02T10:00:00Z"
      }
    }
  }'
```

#### Test Slack Simulator

```bash
curl -X POST http://localhost:8082/services/T00000000/B00000000/test \
  -H "Content-Type: application/json" \
  -d '{
    "text": "Test message from curl",
    "channel": "#test",
    "username": "Test Bot",
    "icon_emoji": ":robot_face:",
    "attachments": [{
      "color": "good",
      "title": "Test Attachment",
      "text": "This is a test attachment"
    }]
  }'
```

### Clear Service Data

```bash
# Clear Jira tickets
curl -X POST http://localhost:8080/tickets/clear

# Clear PagerDuty incidents
curl -X POST http://localhost:8081/incidents/clear

# Clear Slack messages
curl -X POST http://localhost:8082/messages/clear
```

## 🐛 Troubleshooting

### Services Not Starting

```bash
# Check Docker is running
docker ps

# View logs
docker-compose logs

# Rebuild services
docker-compose down
docker-compose up -d --build
```

### Port Already in Use

```bash
# Find process using port 8080
lsof -i :8080

# Kill the process
kill -9 <PID>

# Or change ports in docker-compose.yml
```

### Demo App Won't Build

```bash
# Clean and rebuild
cd demo
rm -rf build
cd ..
./build.sh --clean
cd demo
./run-demo.sh build
```

### Services Unhealthy

```bash
# Check service health
curl http://localhost:8080/health
curl http://localhost:8081/health
curl http://localhost:8082/health

# Wait longer for services to start
sleep 10
```

## 📚 Understanding the Output

### Jira Tickets Created When:
- Fraud detection triggers (5% of transactions)
- Critical errors occur
- Pattern of repeated failures detected

### PagerDuty Incidents Created When:
- Anomaly score > 0.9
- Latency > 2000ms
- Service becomes unavailable

### Slack Notifications Sent For:
- All errors and warnings
- Anomaly detections (score > 0.85)
- Pattern detections
- Periodic summaries (every 20 transactions)

## 🎓 Learning Resources

### Explore the Code

- **Simulators**: `simulators/*/server.py` - Python Flask servers
- **Demo App**: `payment_demo.cpp` - C++ payment service simulation
- **Config**: `config.json` - Integration configuration
- **Docker**: `docker-compose.yml` - Service orchestration

### Experiment

1. **Modify Transaction Rates**: Change sleep duration in `payment_demo.cpp`
2. **Adjust Failure Rates**: Modify probabilities in payment processing logic
3. **Add New Scenarios**: Implement additional failure modes
4. **Custom Integrations**: Add your own webhook endpoints

## 🤝 Contributing

To add new simulators or improve existing ones:

1. Create a new directory in `simulators/`
2. Add Dockerfile and server implementation
3. Update `docker-compose.yml`
4. Update this README

## 📄 License

Same as AgentLog main project.

## 🎉 Next Steps

After running the demo:

1. Explore the web dashboard at http://localhost:3000
2. Watch the colorful console output from simulators
3. Modify `config.json` to adjust thresholds
4. Edit `payment_demo.cpp` to add new scenarios
5. Integrate AgentLog into your own C++ applications!

## 📞 Support

For issues or questions:
- Check the troubleshooting section above
- View service logs: `docker-compose logs`
- Open an issue on GitHub

---

**Enjoy exploring AgentLog! 🚀**
