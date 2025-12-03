#!/usr/bin/env python3
"""
Mock Slack Server - Simulates Slack Incoming Webhooks
Receives notifications from AgentLog and displays them beautifully
"""

from flask import Flask, request, jsonify, render_template
import json
import os
from datetime import datetime
from colorama import Fore, Back, Style, init

# Initialize colorama
init(autoreset=True)

app = Flask(__name__)

# Store messages in memory
messages = []

def print_banner():
    """Print startup banner"""
    print(f"\n{Back.MAGENTA}{Fore.WHITE}{'=' * 80}{Style.RESET_ALL}")
    print(f"{Fore.MAGENTA}💬  Mock Slack Server - Notification Simulator{Style.RESET_ALL}")
    print(f"{Back.MAGENTA}{Fore.WHITE}{'=' * 80}{Style.RESET_ALL}\n")
    print(f"{Fore.GREEN}✓ Server running on port {os.getenv('PORT', 8082)}{Style.RESET_ALL}")
    print(f"{Fore.GREEN}✓ Ready to receive Slack notifications{Style.RESET_ALL}")
    print(f"{Fore.YELLOW}➜ Endpoint: POST /services/*/incoming-webhook{Style.RESET_ALL}\n")

def get_color_emoji(color):
    """Get emoji based on color"""
    color_map = {
        'good': ('🟢', Fore.GREEN),
        'warning': ('🟡', Fore.YELLOW),
        'danger': ('🔴', Fore.RED),
        '#36a64f': ('🟢', Fore.GREEN),
        '#ff0000': ('🔴', Fore.RED),
        '#ffcc00': ('🟡', Fore.YELLOW),
    }
    return color_map.get(color, ('🔵', Fore.BLUE))

def print_message(message):
    """Print Slack message in a beautiful format"""
    
    print(f"\n{Back.MAGENTA}{Fore.BLACK} NEW SLACK MESSAGE {Style.RESET_ALL}")
    print(f"{Fore.MAGENTA}{'─' * 80}{Style.RESET_ALL}")
    
    # Channel (if specified)
    channel = message.get('channel', '#general')
    print(f"{Fore.WHITE}Channel:     {Fore.CYAN}{channel}{Style.RESET_ALL}")
    
    # Username
    username = message.get('username', 'AgentLog Bot')
    print(f"{Fore.WHITE}From:        {Fore.YELLOW}{username}{Style.RESET_ALL}")
    
    # Icon
    icon_emoji = message.get('icon_emoji', ':robot_face:')
    icon_url = message.get('icon_url', '')
    if icon_url:
        print(f"{Fore.WHITE}Icon:        {Fore.BLUE}{icon_url}{Style.RESET_ALL}")
    else:
        print(f"{Fore.WHITE}Icon:        {Fore.BLUE}{icon_emoji}{Style.RESET_ALL}")
    
    # Text
    text = message.get('text', '')
    if text:
        print(f"{Fore.WHITE}Message:     {Fore.WHITE}{text}{Style.RESET_ALL}")
    
    # Attachments
    attachments = message.get('attachments', [])
    if attachments:
        print(f"\n{Fore.WHITE}Attachments:{Style.RESET_ALL}")
        for i, att in enumerate(attachments, 1):
            color = att.get('color', 'good')
            emoji, color_code = get_color_emoji(color)
            
            print(f"\n  {emoji} {color_code}Attachment {i}{Style.RESET_ALL}")
            
            # Pretext
            pretext = att.get('pretext', '')
            if pretext:
                print(f"  {Fore.LIGHTBLACK_EX}{pretext}{Style.RESET_ALL}")
            
            # Author
            author_name = att.get('author_name', '')
            if author_name:
                print(f"  {Fore.CYAN}👤 {author_name}{Style.RESET_ALL}")
            
            # Title
            title = att.get('title', '')
            title_link = att.get('title_link', '')
            if title:
                if title_link:
                    print(f"  {Fore.YELLOW}📌 {title} ({title_link}){Style.RESET_ALL}")
                else:
                    print(f"  {Fore.YELLOW}📌 {title}{Style.RESET_ALL}")
            
            # Text
            att_text = att.get('text', '')
            if att_text:
                # Truncate long text
                if len(att_text) > 300:
                    att_text = att_text[:300] + "..."
                print(f"  {Fore.WHITE}{att_text}{Style.RESET_ALL}")
            
            # Fields
            fields = att.get('fields', [])
            if fields:
                print(f"  {Fore.WHITE}Fields:{Style.RESET_ALL}")
                for field in fields:
                    field_title = field.get('title', '')
                    field_value = field.get('value', '')
                    short = field.get('short', False)
                    short_indicator = " (short)" if short else ""
                    print(f"    {Fore.LIGHTBLACK_EX}• {field_title}:{short_indicator} {Fore.WHITE}{field_value}{Style.RESET_ALL}")
            
            # Footer
            footer = att.get('footer', '')
            footer_icon = att.get('footer_icon', '')
            timestamp = att.get('ts', '')
            if footer or timestamp:
                footer_text = footer
                if timestamp:
                    dt = datetime.fromtimestamp(int(timestamp))
                    footer_text += f" | {dt.strftime('%Y-%m-%d %H:%M:%S')}"
                print(f"  {Fore.LIGHTBLACK_EX}⎯ {footer_text}{Style.RESET_ALL}")
    
    # Blocks (Slack Block Kit)
    blocks = message.get('blocks', [])
    if blocks:
        print(f"\n{Fore.WHITE}Blocks:{Style.RESET_ALL}")
        for block in blocks:
            block_type = block.get('type', 'unknown')
            
            if block_type == 'section':
                text_obj = block.get('text', {})
                text_content = text_obj.get('text', '')
                print(f"  {Fore.WHITE}• {text_content}{Style.RESET_ALL}")
            
            elif block_type == 'header':
                text_obj = block.get('text', {})
                text_content = text_obj.get('text', '')
                print(f"  {Fore.YELLOW}▸ {text_content}{Style.RESET_ALL}")
            
            elif block_type == 'divider':
                print(f"  {Fore.LIGHTBLACK_EX}{'─' * 60}{Style.RESET_ALL}")
    
    # Timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"\n{Fore.WHITE}Received:    {Fore.GREEN}{timestamp}{Style.RESET_ALL}")
    
    print(f"{Fore.MAGENTA}{'─' * 80}{Style.RESET_ALL}")
    print(f"{Fore.GREEN}✓ Total messages received: {len(messages)}{Style.RESET_ALL}\n")

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({"status": "healthy", "service": "mock-slack"}), 200

@app.route('/services/<path:webhook_path>', methods=['POST'])
def incoming_webhook(webhook_path):
    """Simulate Slack incoming webhook"""
    try:
        data = request.json
        
        # Store message
        message = {
            'webhook_path': webhook_path,
            'timestamp': datetime.now().isoformat(),
            **data
        }
        messages.append(message)
        
        # Print beautiful output
        print_message(message)
        
        # Return Slack-like response
        return jsonify({'ok': True}), 200
        
    except Exception as e:
        print(f"{Fore.RED}✗ Error processing message: {str(e)}{Style.RESET_ALL}")
        return jsonify({'ok': False, 'error': str(e)}), 400

@app.route('/api/chat.postMessage', methods=['POST'])
def post_message():
    """Simulate Slack chat.postMessage API"""
    try:
        data = request.json
        
        # Store message
        message = {
            'api': 'chat.postMessage',
            'timestamp': datetime.now().isoformat(),
            **data
        }
        messages.append(message)
        
        # Print beautiful output
        print_message(message)
        
        # Return Slack-like response
        return jsonify({
            'ok': True,
            'channel': data.get('channel', 'C1234567890'),
            'ts': str(int(datetime.now().timestamp())),
            'message': {
                'text': data.get('text', ''),
                'username': data.get('username', 'bot'),
                'bot_id': 'B1234567890',
                'type': 'message',
                'subtype': 'bot_message',
            }
        }), 200
        
    except Exception as e:
        print(f"{Fore.RED}✗ Error posting message: {str(e)}{Style.RESET_ALL}")
        return jsonify({'ok': False, 'error': str(e)}), 400

@app.route('/messages', methods=['GET'])
def list_messages():
    """List all messages (custom endpoint for debugging)"""
    return jsonify(messages), 200

@app.route('/messages/clear', methods=['POST'])
def clear_messages():
    """Clear all messages (custom endpoint for testing)"""
    global messages
    count = len(messages)
    messages = []
    print(f"{Fore.YELLOW}⚠ Cleared {count} messages{Style.RESET_ALL}")
    return jsonify({'message': f'Cleared {count} messages'}), 200

@app.route('/', methods=['GET'])
def index():
    """Web UI to view all messages"""
    return render_template('index.html')

if __name__ == '__main__':
    print_banner()
    port = int(os.getenv('PORT', 8082))
    app.run(host='0.0.0.0', port=port, debug=False)
