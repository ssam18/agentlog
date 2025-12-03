#!/bin/bash
#
# AgentLog Demo Runner
# Starts all simulators and runs the payment demo application
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Banner
print_banner() {
    echo -e "${CYAN}"
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║                                                              ║"
    echo "║           AgentLog End-to-End Demo Environment              ║"
    echo "║                                                              ║"
    echo "║  Simulated Services:                                         ║"
    echo "║    • Jira Ticket Creation API                                ║"
    echo "║    • PagerDuty Incident Management API                       ║"
    echo "║    • Slack Incoming Webhooks                                 ║"
    echo "║    • RocksDB Persistent Storage                              ║"
    echo "║    • Real-time Dashboard                                     ║"
    echo "║                                                              ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

# Print section header
print_section() {
    echo -e "\n${BLUE}▶ $1${NC}"
    echo -e "${BLUE}$( printf '─%.0s' {1..60} )${NC}"
}

# Check if docker and docker-compose are installed
check_prerequisites() {
    print_section "Checking Prerequisites"
    
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}✗ Docker is not installed${NC}"
        echo "  Please install Docker: https://docs.docker.com/get-docker/"
        exit 1
    fi
    echo -e "${GREEN}✓ Docker is installed${NC}"
    
    if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
        echo -e "${RED}✗ Docker Compose is not installed${NC}"
        echo "  Please install Docker Compose: https://docs.docker.com/compose/install/"
        exit 1
    fi
    echo -e "${GREEN}✓ Docker Compose is installed${NC}"
    
    if ! command -v cmake &> /dev/null; then
        echo -e "${YELLOW}⚠ CMake is not installed (needed to build demo app)${NC}"
    else
        echo -e "${GREEN}✓ CMake is installed${NC}"
    fi
}

# Build AgentLog library
build_agentlog() {
    print_section "Building AgentLog Library"
    
    cd "$(dirname "$0")/.."
    
    if [ ! -d "build" ]; then
        echo -e "${YELLOW}Building AgentLog for the first time...${NC}"
        ./build.sh --type Release
    else
        echo -e "${GREEN}✓ AgentLog already built${NC}"
    fi
    
    cd - > /dev/null
}

# Build demo application
build_demo_app() {
    print_section "Building Demo Application"
    
    cd "$(dirname "$0")"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure and build
    echo -e "${CYAN}Configuring demo application...${NC}"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    echo -e "${CYAN}Building demo application...${NC}"
    cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ -f "payment_demo" ]; then
        echo -e "${GREEN}✓ Demo application built successfully${NC}"
    else
        echo -e "${RED}✗ Failed to build demo application${NC}"
        exit 1
    fi
    
    cd - > /dev/null
}

# Start Docker services
start_services() {
    print_section "Starting Docker Services"
    
    cd "$(dirname "$0")"
    
    # Check if services are already running
    if docker-compose ps | grep -q "Up"; then
        echo -e "${YELLOW}Services are already running. Restarting...${NC}"
        docker-compose down
    fi
    
    echo -e "${CYAN}Starting Jira simulator...${NC}"
    echo -e "${CYAN}Starting PagerDuty simulator...${NC}"
    echo -e "${CYAN}Starting Slack simulator...${NC}"
    echo -e "${CYAN}Starting Dashboard...${NC}"
    
    # Start services
    docker-compose up -d --build
    
    # Wait for services to be healthy
    echo -e "\n${CYAN}Waiting for services to be ready...${NC}"
    sleep 5
    
    # Check service health
    local all_healthy=true
    
    if curl -sf http://localhost:8080/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Jira simulator is ready${NC}"
    else
        echo -e "${RED}✗ Jira simulator is not responding${NC}"
        all_healthy=false
    fi
    
    if curl -sf http://localhost:8081/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ PagerDuty simulator is ready${NC}"
    else
        echo -e "${RED}✗ PagerDuty simulator is not responding${NC}"
        all_healthy=false
    fi
    
    if curl -sf http://localhost:8082/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Slack simulator is ready${NC}"
    else
        echo -e "${RED}✗ Slack simulator is not responding${NC}"
        all_healthy=false
    fi
    
    if curl -sf http://localhost:3000 > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Dashboard is ready${NC}"
    else
        echo -e "${YELLOW}⚠ Dashboard is not responding (may take a few more seconds)${NC}"
    fi
    
    if [ "$all_healthy" = false ]; then
        echo -e "\n${YELLOW}Some services are not ready. Check logs with: docker-compose logs${NC}"
    fi
}

# Show service URLs
show_urls() {
    print_section "Service URLs"
    
    echo -e "${GREEN}📊 Dashboard:        ${CYAN}http://localhost:3000${NC}"
    echo -e "${GREEN}🎫 Jira API:         ${CYAN}http://localhost:8080${NC}"
    echo -e "${GREEN}🚨 PagerDuty API:    ${CYAN}http://localhost:8081${NC}"
    echo -e "${GREEN}💬 Slack Webhook:    ${CYAN}http://localhost:8082${NC}"
    echo ""
    echo -e "${YELLOW}Open the dashboard in your browser to see real-time updates!${NC}"
}

# Run demo application
run_demo() {
    print_section "Running Payment Service Demo"
    
    cd "$(dirname "$0")/build"
    
    echo -e "${CYAN}"
    echo "The demo application will simulate a payment processing service."
    echo "It will generate various events including:"
    echo "  • Successful payments"
    echo "  • Payment failures"
    echo "  • High latency events (triggers PagerDuty)"
    echo "  • Fraud detection (creates Jira tickets)"
    echo "  • Database errors"
    echo "  • API timeouts"
    echo ""
    echo "Watch the simulators' console output and the web dashboard!"
    echo -e "${NC}"
    
    echo -e "${GREEN}Press Ctrl+C to stop the demo${NC}\n"
    sleep 2
    
    # Run the demo
    ./payment_demo
}

# View logs
view_logs() {
    print_section "Viewing Service Logs"
    
    cd "$(dirname "$0")"
    
    echo -e "${CYAN}Opening logs in follow mode (press Ctrl+C to stop)${NC}\n"
    docker-compose logs -f
}

# Stop services
stop_services() {
    print_section "Stopping Services"
    
    cd "$(dirname "$0")"
    
    echo -e "${YELLOW}Stopping all services...${NC}"
    docker-compose down
    
    echo -e "${GREEN}✓ All services stopped${NC}"
}

# Cleanup
cleanup() {
    print_section "Cleanup"
    
    cd "$(dirname "$0")"
    
    echo -e "${YELLOW}Removing containers, networks, and volumes...${NC}"
    docker-compose down -v
    
    echo -e "${YELLOW}Removing build artifacts...${NC}"
    rm -rf build
    
    echo -e "${GREEN}✓ Cleanup complete${NC}"
}

# Main menu
show_menu() {
    echo -e "\n${MAGENTA}Choose an option:${NC}"
    echo "  1) Start demo (build + run)"
    echo "  2) Build only"
    echo "  3) Start services only"
    echo "  4) Run demo app (services must be running)"
    echo "  5) View logs"
    echo "  6) Stop services"
    echo "  7) Cleanup (stop + remove volumes)"
    echo "  8) Exit"
    echo -e -n "\n${CYAN}Enter choice [1-8]: ${NC}"
}

# Main function
main() {
    print_banner
    
    # Parse command line arguments
    case "${1:-menu}" in
        start|run)
            check_prerequisites
            build_agentlog
            build_demo_app
            start_services
            show_urls
            echo -e "\n${YELLOW}Waiting 3 seconds before starting demo app...${NC}"
            sleep 3
            run_demo
            ;;
        build)
            check_prerequisites
            build_agentlog
            build_demo_app
            ;;
        services)
            check_prerequisites
            start_services
            show_urls
            ;;
        demo)
            run_demo
            ;;
        logs)
            view_logs
            ;;
        stop)
            stop_services
            ;;
        cleanup)
            cleanup
            ;;
        menu)
            check_prerequisites
            while true; do
                show_menu
                read -r choice
                case $choice in
                    1)
                        build_agentlog
                        build_demo_app
                        start_services
                        show_urls
                        echo -e "\n${YELLOW}Waiting 3 seconds before starting demo app...${NC}"
                        sleep 3
                        run_demo
                        ;;
                    2)
                        build_agentlog
                        build_demo_app
                        ;;
                    3)
                        start_services
                        show_urls
                        ;;
                    4)
                        run_demo
                        ;;
                    5)
                        view_logs
                        ;;
                    6)
                        stop_services
                        ;;
                    7)
                        cleanup
                        ;;
                    8)
                        echo -e "${GREEN}Goodbye!${NC}"
                        exit 0
                        ;;
                    *)
                        echo -e "${RED}Invalid option${NC}"
                        ;;
                esac
            done
            ;;
        *)
            echo "Usage: $0 {start|build|services|demo|logs|stop|cleanup|menu}"
            echo ""
            echo "Commands:"
            echo "  start    - Build everything and run complete demo"
            echo "  build    - Build AgentLog and demo application"
            echo "  services - Start Docker services only"
            echo "  demo     - Run demo application (requires services running)"
            echo "  logs     - View service logs"
            echo "  stop     - Stop all services"
            echo "  cleanup  - Stop services and remove all data"
            echo "  menu     - Show interactive menu (default)"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
