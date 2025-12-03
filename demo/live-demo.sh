#!/bin/bash
#
# AgentLog Live Demo - Complete Interactive Demonstration
# 
# This script will:
# 1. Start all Docker services (Jira, PagerDuty, Slack, Dashboard, RocksDB)
# 2. Wait for all services to be healthy
# 3. Build the demo C++ application
# 4. Open web browsers to show all UIs
# 5. Run the demo application to generate live events
#
# Usage: ./live-demo.sh [duration_in_seconds]
#   Example: ./live-demo.sh 60    # Run for 60 seconds
#   Default: 30 seconds
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Configuration
DEMO_DURATION=${1:-30}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Service URLs
JIRA_URL="http://localhost:8080"
PAGERDUTY_URL="http://localhost:8081"
SLACK_URL="http://localhost:8082"
DASHBOARD_URL="http://localhost:3000"

# Print banner
print_banner() {
    echo ""
    echo -e "${BOLD}${MAGENTA}╔════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${MAGENTA}║                                                                    ║${NC}"
    echo -e "${BOLD}${MAGENTA}║          🎬  AgentLog Live Demo - Interactive Showcase  🎬         ║${NC}"
    echo -e "${BOLD}${MAGENTA}║                                                                    ║${NC}"
    echo -e "${BOLD}${MAGENTA}╚════════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

# Print section header
print_section() {
    echo ""
    echo -e "${BOLD}${CYAN}▶ $1${NC}"
    echo -e "${CYAN}────────────────────────────────────────────────────────────────${NC}"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Wait for service to be healthy
wait_for_service() {
    local url=$1
    local name=$2
    local max_attempts=30
    local attempt=0
    
    echo -n "Waiting for ${name}..."
    while [ $attempt -lt $max_attempts ]; do
        if curl -s "${url}/health" > /dev/null 2>&1; then
            echo -e " ${GREEN}✓ Ready${NC}"
            return 0
        fi
        echo -n "."
        sleep 1
        ((attempt++))
    done
    
    echo -e " ${RED}✗ Failed${NC}"
    return 1
}

# Open URL in browser
open_browser() {
    local url=$1
    local name=$2
    
    echo -e "${YELLOW}🌐 Opening ${name}: ${url}${NC}"
    
    if command_exists xdg-open; then
        xdg-open "$url" >/dev/null 2>&1 &
    elif command_exists open; then
        open "$url" >/dev/null 2>&1 &
    elif command_exists firefox; then
        firefox "$url" >/dev/null 2>&1 &
    elif command_exists google-chrome; then
        google-chrome "$url" >/dev/null 2>&1 &
    else
        echo -e "${YELLOW}⚠ Could not open browser automatically. Please open: ${url}${NC}"
    fi
    
    sleep 1
}

# Cleanup function
cleanup() {
    echo ""
    print_section "Cleaning up"
    echo -e "${YELLOW}Stopping demo application...${NC}"
    echo -e "${GREEN}✓ Demo stopped${NC}"
    echo ""
    echo -e "${CYAN}${BOLD}Services are still running. You can:${NC}"
    echo -e "  • View Jira tickets:        ${JIRA_URL}"
    echo -e "  • View PagerDuty incidents: ${PAGERDUTY_URL}"
    echo -e "  • View Slack messages:      ${SLACK_URL}"
    echo -e "  • View Dashboard:           ${DASHBOARD_URL}"
    echo ""
    echo -e "${YELLOW}To stop all services: ${BOLD}docker compose down${NC}"
    echo ""
}

# Set trap for cleanup
trap cleanup EXIT INT TERM

# Main script
main() {
    print_banner
    
    # Check prerequisites
    print_section "Checking prerequisites"
    
    if ! command_exists docker; then
        echo -e "${RED}✗ Docker is not installed${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Docker found${NC}"
    
    if ! command_exists cmake; then
        echo -e "${RED}✗ CMake is not installed${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ CMake found${NC}"
    
    # Start Docker services
    print_section "Starting Docker services"
    cd "${SCRIPT_DIR}"
    
    echo "Starting containers..."
    docker compose up -d
    echo -e "${GREEN}✓ Containers started${NC}"
    
    # Wait for services to be healthy
    print_section "Waiting for services to be ready"
    
    wait_for_service "${JIRA_URL}" "Mock Jira" || exit 1
    wait_for_service "${PAGERDUTY_URL}" "Mock PagerDuty" || exit 1
    wait_for_service "${SLACK_URL}" "Mock Slack" || exit 1
    wait_for_service "${DASHBOARD_URL}" "Dashboard" || exit 1
    
    echo ""
    echo -e "${GREEN}${BOLD}✓ All services are ready!${NC}"
    
    # Build demo application
    print_section "Building demo application"
    
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    echo "Running CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
    echo -e "${GREEN}✓ CMake configured${NC}"
    
    echo "Compiling..."
    cmake --build . -j$(nproc) >/dev/null 2>&1
    echo -e "${GREEN}✓ Build complete${NC}"
    
    # Open browsers
    print_section "Opening web interfaces"
    
    sleep 2  # Give user time to read
    
    open_browser "${DASHBOARD_URL}" "Dashboard"
    sleep 1
    open_browser "${JIRA_URL}" "Jira Tickets"
    sleep 1
    open_browser "${PAGERDUTY_URL}" "PagerDuty Incidents"
    sleep 1
    open_browser "${SLACK_URL}" "Slack Messages"
    
    echo ""
    echo -e "${GREEN}${BOLD}✓ All web interfaces opened${NC}"
    
    # Instructions
    print_section "Live Demo Starting"
    
    echo ""
    echo -e "${CYAN}${BOLD}📺 WATCH THE MAGIC HAPPEN! 📺${NC}"
    echo ""
    echo -e "The demo will now run for ${BOLD}${DEMO_DURATION} seconds${NC} and show:"
    echo ""
    echo -e "  ${GREEN}✅ Successful payment transactions${NC}"
    echo -e "  ${YELLOW}⚠️  Payment warnings (insufficient funds)${NC}"
    echo -e "  ${RED}🔴 Timeout alerts → PagerDuty + Slack${NC}"
    echo -e "  ${MAGENTA}🚫 Fraud detection → Jira + PagerDuty + Slack${NC}"
    echo ""
    echo -e "${CYAN}Watch your browser windows to see events appear in real-time!${NC}"
    echo ""
    echo -e "${BOLD}Console output below:${NC}"
    echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    # Run demo application
    cd "${BUILD_DIR}"
    timeout ${DEMO_DURATION} ./simple_payment_demo || true
    
    # Show summary
    echo ""
    echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
    echo ""
    
    print_section "Demo Summary"
    
    # Fetch statistics
    echo ""
    echo -e "${BOLD}Fetching statistics from services...${NC}"
    echo ""
    
    # Get stats from each service
    JIRA_COUNT=$(curl -s ${JIRA_URL}/tickets | python3 -c "import sys, json; data = json.load(sys.stdin); print(len(data))" 2>/dev/null || echo "0")
    PD_COUNT=$(curl -s ${PAGERDUTY_URL}/incidents | python3 -c "import sys, json; data = json.load(sys.stdin); print(data['total'])" 2>/dev/null || echo "0")
    SLACK_COUNT=$(curl -s ${SLACK_URL}/messages | python3 -c "import sys, json; data = json.load(sys.stdin); print(len(data))" 2>/dev/null || echo "0")
    
    echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${CYAN}║                    DEMO STATISTICS                          ║${NC}"
    echo -e "${BOLD}${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${BOLD}${CYAN}║${NC}  🎫 Jira Tickets Created:        ${BOLD}${GREEN}${JIRA_COUNT}${NC}"
    echo -e "${BOLD}${CYAN}║${NC}  🚨 PagerDuty Incidents:          ${BOLD}${YELLOW}${PD_COUNT}${NC}"
    echo -e "${BOLD}${CYAN}║${NC}  💬 Slack Messages Sent:          ${BOLD}${BLUE}${SLACK_COUNT}${NC}"
    echo -e "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    # Instructions for exploring
    print_section "Explore the Results"
    
    echo ""
    echo -e "The web interfaces are still open. You can:"
    echo ""
    echo -e "  ${CYAN}1. View ticket details in Jira:        ${BOLD}${JIRA_URL}${NC}"
    echo -e "  ${CYAN}2. Check incident statuses in PagerDuty: ${BOLD}${PAGERDUTY_URL}${NC}"
    echo -e "  ${CYAN}3. Read message threads in Slack:      ${BOLD}${SLACK_URL}${NC}"
    echo -e "  ${CYAN}4. See aggregated view in Dashboard:   ${BOLD}${DASHBOARD_URL}${NC}"
    echo ""
    echo -e "  ${GREEN}5. Run the demo again:                 ${BOLD}./live-demo.sh${NC}"
    echo -e "  ${YELLOW}6. Clear all data:                     ${BOLD}docker compose restart${NC}"
    echo -e "  ${RED}7. Stop all services:                  ${BOLD}docker compose down${NC}"
    echo ""
    
    # Show container logs info
    echo -e "${CYAN}${BOLD}💡 Pro Tips:${NC}"
    echo ""
    echo -e "  • Watch colorful console output: ${BOLD}docker compose logs -f mock-jira${NC}"
    echo -e "  • View all service logs:         ${BOLD}docker compose logs -f${NC}"
    echo -e "  • Restart just the demo:         ${BOLD}cd build && ./simple_payment_demo${NC}"
    echo ""
    
    print_section "Demo Complete"
    
    echo ""
    echo -e "${GREEN}${BOLD}✅ Live demo completed successfully!${NC}"
    echo ""
    echo -e "${MAGENTA}${BOLD}Thank you for watching the AgentLog demonstration! 🎉${NC}"
    echo ""
}

# Run main function
main
