#!/bin/bash

# Integration Test Setup Script
# This script helps you configure and test the AgentLog external integrations

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build/examples"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  AgentLog External Integration Test Setup${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check if test_integrations exists
if [ ! -f "$BUILD_DIR/test_integrations" ]; then
    echo -e "${RED}Error: test_integrations executable not found${NC}"
    echo "Please build the project first:"
    echo "  cd build && cmake .. && make test_integrations"
    exit 1
fi

echo -e "${GREEN}✓${NC} test_integrations executable found"
echo

# Check current configuration
echo -e "${YELLOW}Current Configuration:${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo

# Jira
echo -e "${BLUE}Jira Cloud Integration:${NC}"
if [ -n "$JIRA_URL" ]; then
    echo -e "  JIRA_URL:         ${GREEN}✓${NC} $JIRA_URL"
else
    echo -e "  JIRA_URL:         ${RED}✗${NC} Not set"
fi

if [ -n "$JIRA_USERNAME" ]; then
    echo -e "  JIRA_USERNAME:    ${GREEN}✓${NC} $JIRA_USERNAME"
else
    echo -e "  JIRA_USERNAME:    ${RED}✗${NC} Not set"
fi

if [ -n "$JIRA_API_TOKEN" ]; then
    echo -e "  JIRA_API_TOKEN:   ${GREEN}✓${NC} [hidden]"
else
    echo -e "  JIRA_API_TOKEN:   ${RED}✗${NC} Not set"
fi

if [ -n "$JIRA_PROJECT_KEY" ]; then
    echo -e "  JIRA_PROJECT_KEY: ${GREEN}✓${NC} $JIRA_PROJECT_KEY"
else
    echo -e "  JIRA_PROJECT_KEY: ${RED}✗${NC} Not set"
fi
echo

# PagerDuty
echo -e "${BLUE}PagerDuty Integration:${NC}"
if [ -n "$PAGERDUTY_INTEGRATION_KEY" ]; then
    echo -e "  INTEGRATION_KEY:  ${GREEN}✓${NC} [hidden]"
else
    echo -e "  INTEGRATION_KEY:  ${RED}✗${NC} Not set"
fi
echo

# Slack
echo -e "${BLUE}Slack Integration:${NC}"
if [ -n "$SLACK_WEBHOOK_URL" ]; then
    echo -e "  WEBHOOK_URL:      ${GREEN}✓${NC} [hidden]"
else
    echo -e "  WEBHOOK_URL:      ${RED}✗${NC} Not set"
fi

if [ -n "$SLACK_CHANNEL" ]; then
    echo -e "  CHANNEL:          ${GREEN}✓${NC} $SLACK_CHANNEL"
else
    echo -e "  CHANNEL:          ${YELLOW}○${NC} Using default (optional)"
fi
echo

# Count configured integrations
CONFIGURED=0
[ -n "$JIRA_URL" ] && [ -n "$JIRA_USERNAME" ] && [ -n "$JIRA_API_TOKEN" ] && [ -n "$JIRA_PROJECT_KEY" ] && ((CONFIGURED++))
[ -n "$PAGERDUTY_INTEGRATION_KEY" ] && ((CONFIGURED++))
[ -n "$SLACK_WEBHOOK_URL" ] && ((CONFIGURED++))

echo -e "${YELLOW}Summary:${NC} $CONFIGURED/3 integrations configured"
echo

# Test options
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Test Options${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo
echo "  1) Run demo test (no real API calls)"
echo "  2) Run live test (requires credentials)"
echo "  3) Configure Jira"
echo "  4) Configure PagerDuty"
echo "  5) Configure Slack"
echo "  6) Show setup instructions"
echo "  7) Exit"
echo

read -p "Select option [1-7]: " choice

case $choice in
    1)
        echo
        echo -e "${GREEN}Running demo test...${NC}"
        echo
        cd "$BUILD_DIR"
        ./test_integrations
        ;;
    2)
        if [ $CONFIGURED -eq 0 ]; then
            echo
            echo -e "${YELLOW}Warning: No integrations configured!${NC}"
            echo "The test will run but no notifications will be sent."
            echo
            read -p "Continue anyway? [y/N]: " confirm
            if [[ ! $confirm =~ ^[Yy]$ ]]; then
                exit 0
            fi
        fi
        echo
        echo -e "${GREEN}Running live test...${NC}"
        echo -e "${YELLOW}This will make real API calls to configured services!${NC}"
        echo
        cd "$BUILD_DIR"
        ./test_integrations --live
        ;;
    3)
        echo
        echo -e "${BLUE}Jira Cloud Configuration:${NC}"
        echo
        read -p "Jira URL (e.g., https://yourcompany.atlassian.net): " jira_url
        read -p "Jira Username/Email: " jira_username
        read -sp "Jira API Token: " jira_token
        echo
        read -p "Jira Project Key (e.g., PROJ): " jira_project
        echo
        echo "Add these to your ~/.bashrc or ~/.zshrc:"
        echo
        echo "  export JIRA_URL=\"$jira_url\""
        echo "  export JIRA_USERNAME=\"$jira_username\""
        echo "  export JIRA_API_TOKEN=\"$jira_token\""
        echo "  export JIRA_PROJECT_KEY=\"$jira_project\""
        echo
        ;;
    4)
        echo
        echo -e "${BLUE}PagerDuty Configuration:${NC}"
        echo
        read -sp "PagerDuty Integration Key: " pd_key
        echo
        echo
        echo "Add this to your ~/.bashrc or ~/.zshrc:"
        echo
        echo "  export PAGERDUTY_INTEGRATION_KEY=\"$pd_key\""
        echo
        ;;
    5)
        echo
        echo -e "${BLUE}Slack Configuration:${NC}"
        echo
        read -p "Slack Webhook URL: " slack_url
        read -p "Slack Channel (optional, e.g., #incidents): " slack_channel
        echo
        echo "Add these to your ~/.bashrc or ~/.zshrc:"
        echo
        echo "  export SLACK_WEBHOOK_URL=\"$slack_url\""
        [ -n "$slack_channel" ] && echo "  export SLACK_CHANNEL=\"$slack_channel\""
        echo
        ;;
    6)
        echo
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${BLUE}  Setup Instructions${NC}"
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo
        echo -e "${YELLOW}1. Jira Cloud:${NC}"
        echo "   a) Go to https://id.atlassian.com/manage-profile/security/api-tokens"
        echo "   b) Click 'Create API token'"
        echo "   c) Copy the token"
        echo "   d) Set environment variables:"
        echo "      export JIRA_URL=\"https://yourcompany.atlassian.net\""
        echo "      export JIRA_USERNAME=\"your.email@company.com\""
        echo "      export JIRA_API_TOKEN=\"your-token\""
        echo "      export JIRA_PROJECT_KEY=\"PROJ\""
        echo
        echo -e "${YELLOW}2. PagerDuty:${NC}"
        echo "   a) Go to PagerDuty → Services → Add Integration"
        echo "   b) Select 'Events API V2'"
        echo "   c) Copy the Integration Key"
        echo "   d) Set environment variable:"
        echo "      export PAGERDUTY_INTEGRATION_KEY=\"your-key\""
        echo
        echo -e "${YELLOW}3. Slack:${NC}"
        echo "   a) Go to https://api.slack.com/messaging/webhooks"
        echo "   b) Click 'Create your Slack app'"
        echo "   c) Enable Incoming Webhooks"
        echo "   d) Add webhook to workspace"
        echo "   e) Copy webhook URL"
        echo "   f) Set environment variable:"
        echo "      export SLACK_WEBHOOK_URL=\"https://hooks.slack.com/services/...\""
        echo "      export SLACK_CHANNEL=\"#incidents\"  # Optional"
        echo
        echo -e "${GREEN}After setting environment variables, reload your shell or run:${NC}"
        echo "  source ~/.bashrc  # or source ~/.zshrc"
        echo
        ;;
    7)
        echo "Exiting..."
        exit 0
        ;;
    *)
        echo -e "${RED}Invalid option${NC}"
        exit 1
        ;;
esac
