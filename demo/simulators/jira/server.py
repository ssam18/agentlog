#!/usr/bin/env python3
"""
Mock Jira Server - Simulates Jira REST API for ticket creation
Receives webhook calls from AgentLog and displays them beautifully
"""

from flask import Flask, request, jsonify, render_template
import json
import os
from datetime import datetime
from colorama import Fore, Back, Style, init

# Initialize colorama
init(autoreset=True)

app = Flask(__name__)

# Store tickets in memory
tickets = []
ticket_counter = 1000

def print_banner():
    """Print startup banner"""
    print(f"\n{Back.BLUE}{Fore.WHITE}{'=' * 80}{Style.RESET_ALL}")
    print(f"{Fore.CYAN}🎫  Mock Jira Server - Ticket Creation Simulator{Style.RESET_ALL}")
    print(f"{Back.BLUE}{Fore.WHITE}{'=' * 80}{Style.RESET_ALL}\n")
    print(f"{Fore.GREEN}✓ Server running on port {os.getenv('PORT', 8080)}{Style.RESET_ALL}")
    print(f"{Fore.GREEN}✓ Ready to receive ticket creation requests{Style.RESET_ALL}")
    print(f"{Fore.YELLOW}➜ Endpoint: POST /rest/api/2/issue{Style.RESET_ALL}\n")

def print_ticket(ticket):
    """Print ticket in a beautiful format"""
    global ticket_counter
    
    print(f"\n{Back.GREEN}{Fore.BLACK} NEW TICKET CREATED {Style.RESET_ALL}")
    print(f"{Fore.CYAN}{'─' * 80}{Style.RESET_ALL}")
    
    # Ticket ID
    print(f"{Fore.WHITE}Ticket ID:   {Fore.GREEN}{ticket['key']}{Style.RESET_ALL}")
    
    # Priority with color coding
    priority = ticket['fields'].get('priority', {}).get('name', 'Medium')
    priority_color = {
        'Critical': Fore.RED,
        'High': Fore.YELLOW,
        'Medium': Fore.CYAN,
        'Low': Fore.GREEN
    }.get(priority, Fore.WHITE)
    print(f"{Fore.WHITE}Priority:    {priority_color}{priority}{Style.RESET_ALL}")
    
    # Issue Type
    issue_type = ticket['fields'].get('issuetype', {}).get('name', 'Bug')
    print(f"{Fore.WHITE}Type:        {Fore.MAGENTA}{issue_type}{Style.RESET_ALL}")
    
    # Summary
    summary = ticket['fields'].get('summary', 'No summary')
    print(f"{Fore.WHITE}Summary:     {Fore.YELLOW}{summary}{Style.RESET_ALL}")
    
    # Description
    description = ticket['fields'].get('description', 'No description')
    if len(description) > 200:
        description = description[:200] + "..."
    print(f"{Fore.WHITE}Description: {Fore.WHITE}{description}{Style.RESET_ALL}")
    
    # Labels
    labels = ticket['fields'].get('labels', [])
    if labels:
        print(f"{Fore.WHITE}Labels:      {Fore.BLUE}{', '.join(labels)}{Style.RESET_ALL}")
    
    # Components
    components = ticket['fields'].get('components', [])
    if components:
        comp_names = [c.get('name', '') for c in components]
        print(f"{Fore.WHITE}Components:  {Fore.CYAN}{', '.join(comp_names)}{Style.RESET_ALL}")
    
    # Timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"{Fore.WHITE}Created:     {Fore.GREEN}{timestamp}{Style.RESET_ALL}")
    
    print(f"{Fore.CYAN}{'─' * 80}{Style.RESET_ALL}")
    print(f"{Fore.GREEN}✓ Total tickets created: {len(tickets)}{Style.RESET_ALL}\n")

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({"status": "healthy", "service": "mock-jira"}), 200

@app.route('/rest/api/2/issue', methods=['POST'])
def create_issue():
    """Simulate Jira issue creation"""
    global ticket_counter
    
    try:
        data = request.json
        
        # Generate ticket key
        project = data.get('fields', {}).get('project', {}).get('key', 'AGENT')
        ticket_key = f"{project}-{ticket_counter}"
        ticket_counter += 1
        
        # Create ticket response
        ticket = {
            'id': str(ticket_counter),
            'key': ticket_key,
            'self': f'http://localhost:8080/rest/api/2/issue/{ticket_counter}',
            'fields': data.get('fields', {}),
            'created': datetime.now().isoformat()
        }
        
        # Store ticket
        tickets.append(ticket)
        
        # Print beautiful output
        print_ticket(ticket)
        
        # Return Jira-like response
        return jsonify({
            'id': ticket['id'],
            'key': ticket['key'],
            'self': ticket['self']
        }), 201
        
    except Exception as e:
        print(f"{Fore.RED}✗ Error creating ticket: {str(e)}{Style.RESET_ALL}")
        return jsonify({'error': str(e)}), 400

@app.route('/rest/api/2/issue/<issue_key>', methods=['GET'])
def get_issue(issue_key):
    """Get ticket by key"""
    for ticket in tickets:
        if ticket['key'] == issue_key:
            return jsonify(ticket), 200
    return jsonify({'error': 'Issue not found'}), 404

@app.route('/rest/api/2/search', methods=['GET', 'POST'])
def search_issues():
    """Search tickets"""
    return jsonify({
        'startAt': 0,
        'maxResults': len(tickets),
        'total': len(tickets),
        'issues': tickets
    }), 200

@app.route('/tickets', methods=['GET'])
def list_tickets():
    """List all tickets (custom endpoint for debugging)"""
    return jsonify(tickets), 200

@app.route('/tickets/clear', methods=['POST'])
def clear_tickets():
    """Clear all tickets (custom endpoint for testing)"""
    global tickets, ticket_counter
    count = len(tickets)
    tickets = []
    ticket_counter = 1000
    print(f"{Fore.YELLOW}⚠ Cleared {count} tickets{Style.RESET_ALL}")
    return jsonify({'message': f'Cleared {count} tickets'}), 200

@app.route('/', methods=['GET'])
def index():
    """Web UI to view all tickets"""
    return render_template('index.html')

if __name__ == '__main__':
    print_banner()
    port = int(os.getenv('PORT', 8080))
    app.run(host='0.0.0.0', port=port, debug=False)
