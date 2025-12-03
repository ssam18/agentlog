#!/usr/bin/env python3
"""
Mock PagerDuty Server - Simulates PagerDuty Events API v2
Receives incident alerts from AgentLog and displays them beautifully
"""

from flask import Flask, request, jsonify, render_template
import json
import os
from datetime import datetime
from colorama import Fore, Back, Style, init
import hashlib

# Initialize colorama
init(autoreset=True)

app = Flask(__name__)

# Store incidents in memory
incidents = []
incident_counter = 1

def print_banner():
    """Print startup banner"""
    print(f"\n{Back.RED}{Fore.WHITE}{'=' * 80}{Style.RESET_ALL}")
    print(f"{Fore.RED}🚨  Mock PagerDuty Server - Incident Management Simulator{Style.RESET_ALL}")
    print(f"{Back.RED}{Fore.WHITE}{'=' * 80}{Style.RESET_ALL}\n")
    print(f"{Fore.GREEN}✓ Server running on port {os.getenv('PORT', 8081)}{Style.RESET_ALL}")
    print(f"{Fore.GREEN}✓ Ready to receive incident alerts{Style.RESET_ALL}")
    print(f"{Fore.YELLOW}➜ Endpoint: POST /v2/enqueue{Style.RESET_ALL}\n")

def print_incident(incident, action):
    """Print incident in a beautiful format"""
    
    if action == 'trigger':
        header_color = Back.RED
        action_text = "🚨 INCIDENT TRIGGERED"
    elif action == 'acknowledge':
        header_color = Back.YELLOW
        action_text = "✓ INCIDENT ACKNOWLEDGED"
    elif action == 'resolve':
        header_color = Back.GREEN
        action_text = "✓ INCIDENT RESOLVED"
    else:
        header_color = Back.BLUE
        action_text = f"• INCIDENT {action.upper()}"
    
    print(f"\n{header_color}{Fore.BLACK} {action_text} {Style.RESET_ALL}")
    print(f"{Fore.CYAN}{'─' * 80}{Style.RESET_ALL}")
    
    # Incident Key (Dedup Key)
    dedup_key = incident.get('dedup_key', 'N/A')
    print(f"{Fore.WHITE}Dedup Key:   {Fore.CYAN}{dedup_key}{Style.RESET_ALL}")
    
    # Severity with color coding
    payload = incident.get('payload', {})
    severity = payload.get('severity', 'info')
    severity_color = {
        'critical': Fore.RED,
        'error': Fore.LIGHTRED_EX,
        'warning': Fore.YELLOW,
        'info': Fore.CYAN
    }.get(severity, Fore.WHITE)
    print(f"{Fore.WHITE}Severity:    {severity_color}{severity.upper()}{Style.RESET_ALL}")
    
    # Summary
    summary = payload.get('summary', 'No summary')
    print(f"{Fore.WHITE}Summary:     {Fore.YELLOW}{summary}{Style.RESET_ALL}")
    
    # Source
    source = payload.get('source', 'unknown')
    print(f"{Fore.WHITE}Source:      {Fore.MAGENTA}{source}{Style.RESET_ALL}")
    
    # Timestamp
    timestamp = payload.get('timestamp', datetime.now().isoformat())
    print(f"{Fore.WHITE}Timestamp:   {Fore.GREEN}{timestamp}{Style.RESET_ALL}")
    
    # Component
    component = payload.get('component', '')
    if component:
        print(f"{Fore.WHITE}Component:   {Fore.BLUE}{component}{Style.RESET_ALL}")
    
    # Group
    group = payload.get('group', '')
    if group:
        print(f"{Fore.WHITE}Group:       {Fore.BLUE}{group}{Style.RESET_ALL}")
    
    # Class
    event_class = payload.get('class', '')
    if event_class:
        print(f"{Fore.WHITE}Class:       {Fore.BLUE}{event_class}{Style.RESET_ALL}")
    
    # Custom Details
    custom_details = payload.get('custom_details', {})
    if custom_details:
        print(f"{Fore.WHITE}Details:{Style.RESET_ALL}")
        for key, value in custom_details.items():
            print(f"  {Fore.LIGHTBLACK_EX}• {key}: {Fore.WHITE}{value}{Style.RESET_ALL}")
    
    print(f"{Fore.CYAN}{'─' * 80}{Style.RESET_ALL}")
    
    # Statistics
    triggered = len([i for i in incidents if i.get('status') == 'triggered'])
    acknowledged = len([i for i in incidents if i.get('status') == 'acknowledged'])
    resolved = len([i for i in incidents if i.get('status') == 'resolved'])
    
    print(f"{Fore.RED}■ Triggered: {triggered}  {Fore.YELLOW}■ Acknowledged: {acknowledged}  {Fore.GREEN}■ Resolved: {resolved}{Style.RESET_ALL}\n")

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({"status": "healthy", "service": "mock-pagerduty"}), 200

@app.route('/v2/enqueue', methods=['POST'])
def enqueue_event():
    """Simulate PagerDuty Events API v2"""
    global incident_counter
    
    try:
        data = request.json
        event = data.get('event', {})
        
        # Extract event details
        routing_key = data.get('routing_key', 'unknown')
        event_action = event.get('event_action', 'trigger')
        dedup_key = event.get('dedup_key', f'incident-{incident_counter}')
        payload = event.get('payload', {})
        
        # Find or create incident
        incident = None
        for inc in incidents:
            if inc.get('dedup_key') == dedup_key:
                incident = inc
                break
        
        if not incident:
            incident = {
                'id': incident_counter,
                'dedup_key': dedup_key,
                'status': event_action,
                'payload': payload,
                'routing_key': routing_key,
                'created_at': datetime.now().isoformat(),
                'updated_at': datetime.now().isoformat()
            }
            incidents.append(incident)
            incident_counter += 1
        else:
            incident['status'] = event_action
            incident['updated_at'] = datetime.now().isoformat()
        
        # Print beautiful output
        print_incident(incident, event_action)
        
        # Return PagerDuty-like response
        return jsonify({
            'status': 'success',
            'message': f'Event processed',
            'dedup_key': dedup_key
        }), 202
        
    except Exception as e:
        print(f"{Fore.RED}✗ Error processing event: {str(e)}{Style.RESET_ALL}")
        return jsonify({'status': 'error', 'message': str(e)}), 400

@app.route('/v2/change/enqueue', methods=['POST'])
def enqueue_change():
    """Simulate PagerDuty Change Events API"""
    try:
        data = request.json
        print(f"\n{Back.BLUE}{Fore.WHITE} CHANGE EVENT {Style.RESET_ALL}")
        print(f"{Fore.CYAN}Change: {json.dumps(data, indent=2)}{Style.RESET_ALL}\n")
        
        return jsonify({
            'status': 'success',
            'message': 'Change event processed'
        }), 202
        
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 400

@app.route('/incidents', methods=['GET'])
def list_incidents():
    """List all incidents (custom endpoint for debugging)"""
    # Calculate statistics
    stats = {
        'triggered': sum(1 for i in incidents if i['status'] == 'triggered'),
        'acknowledged': sum(1 for i in incidents if i['status'] == 'acknowledged'),
        'resolved': sum(1 for i in incidents if i['status'] == 'resolved')
    }
    return jsonify({
        'total': len(incidents),
        'incidents': incidents,
        'stats': stats
    }), 200

@app.route('/incidents/clear', methods=['POST'])
def clear_incidents():
    """Clear all incidents (custom endpoint for testing)"""
    global incidents, incident_counter
    count = len(incidents)
    incidents = []
    incident_counter = 1
    print(f"{Fore.YELLOW}⚠ Cleared {count} incidents{Style.RESET_ALL}")
    return jsonify({'message': f'Cleared {count} incidents'}), 200

@app.route('/', methods=['GET'])
def index():
    """Web UI to view all incidents"""
    return render_template('index.html')

if __name__ == '__main__':
    print_banner()
    port = int(os.getenv('PORT', 8081))
    app.run(host='0.0.0.0', port=port, debug=False)
