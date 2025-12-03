#!/usr/bin/env python3
"""
AgentLog Dashboard - Web UI for monitoring all simulators
"""

from flask import Flask, render_template, jsonify
import requests
import os

app = Flask(__name__)

# Service URLs
SERVICES = {
    'jira': 'http://mock-jira:8080',
    'pagerduty': 'http://mock-pagerduty:8081',
    'slack': 'http://mock-slack:8082'
}

@app.route('/')
def index():
    """Render dashboard"""
    return render_template('index.html')

@app.route('/api/stats', methods=['GET'])
def get_stats():
    """Get aggregated stats from all services"""
    stats = {}
    
    # Get Jira tickets
    try:
        resp = requests.get(f"{SERVICES['jira']}/tickets", timeout=2)
        if resp.status_code == 200:
            data = resp.json()
            # Handle both array format and object format
            if isinstance(data, list):
                stats['jira'] = {
                    'total': len(data),
                    'tickets': data[:10]  # Last 10
                }
            else:
                stats['jira'] = {
                    'total': data.get('total', 0),
                    'tickets': data.get('tickets', [])[:10]
                }
    except:
        stats['jira'] = {'total': 0, 'tickets': [], 'error': 'Service unavailable'}
    
    # Get PagerDuty incidents
    try:
        resp = requests.get(f"{SERVICES['pagerduty']}/incidents", timeout=2)
        if resp.status_code == 200:
            data = resp.json()
            stats['pagerduty'] = {
                'total': data.get('total', 0),
                'incidents': data.get('incidents', [])[:10],  # Last 10
                'stats': data.get('stats', {})
            }
    except:
        stats['pagerduty'] = {'total': 0, 'incidents': [], 'error': 'Service unavailable'}
    
    # Get Slack messages
    try:
        resp = requests.get(f"{SERVICES['slack']}/messages", timeout=2)
        if resp.status_code == 200:
            data = resp.json()
            # Handle both array format and object format
            if isinstance(data, list):
                stats['slack'] = {
                    'total': len(data),
                    'messages': data[:10]  # Last 10
                }
            else:
                stats['slack'] = {
                    'total': data.get('total', 0),
                    'messages': data.get('messages', [])[:10]
                }
    except:
        stats['slack'] = {'total': 0, 'messages': [], 'error': 'Service unavailable'}
    
    return jsonify(stats)

@app.route('/api/clear/<service>', methods=['POST'])
def clear_service(service):
    """Clear data for a specific service"""
    if service not in SERVICES:
        return jsonify({'error': 'Unknown service'}), 404
    
    try:
        endpoint_map = {
            'jira': '/tickets/clear',
            'pagerduty': '/incidents/clear',
            'slack': '/messages/clear'
        }
        
        resp = requests.post(f"{SERVICES[service]}{endpoint_map[service]}", timeout=2)
        if resp.status_code == 200:
            return jsonify({'success': True})
        else:
            return jsonify({'error': 'Clear failed'}), 500
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    port = int(os.getenv('PORT', 3000))
    app.run(host='0.0.0.0', port=port, debug=False)
