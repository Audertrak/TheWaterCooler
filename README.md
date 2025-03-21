# TheWaterCooler
We are business people doing business 

## Overview

**The Water Cooler** is an open source business software hub designed to enhance collaboration between employees by providing a platform to work collaboratively on different projects. This project proposal outlines the language-agnostic API for interaction between the hub and different projects.

## Core Components

### Stack Architecture

```
┌───────────────────────────────────────────────────┐
│                 THE WATER COOLER HUB              │
├───────────┬───────────┬───────────┬───────────────┤
│ Discovery │ Company   │ Employee  │ Meeting       │
│ Service   │ Management│ Management│ Management    │
├───────────┼───────────┼───────────┼───────────────┤
│ Message   │ State     │ Version   │ Auth          │
│ Service   │ Sync      │ Control   │ Service       │
├───────────┴───────────┴───────────┴───────────────┤
│              COMMUNICATION LAYER                  │
│         (JSON-RPC over WebSockets/TCP)            │
├───────────────────────────────────────────────────┤
│                Meeting Rooms                      │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐            │
│  │Project 3│  │Project 2│  │Project 3│  ...       │
│  └─────────┘  └─────────┘  └─────────┘            │
└───────────────────────────────────────────────────┘
```

### Dependencies

- **Core Communication**: JSON-RPC over WebSockets/TCP (libp2p, webrtc,
zeroMQ?)
- **Discovery**: mDNS/Bonjour for LAN discovery
- **State Management**: Distributed hash table or CRDT (conflict-free replicated data type)
- **Version Control**: Git-like lightweight version system; project clients
developed/distributed via git repo's (this repo is the official 'store') 
- **Storage**: SQLite/libsql or flat file storage for user/company data
- **Messaging**: Messaging system (mqtt, matrix)
- **Authentication**: Public/private key pair for user identification

## API Specifications

### 1. Hub-Project Registration Interface

#### 1.1 Project Manifest

Every project must provide a manifest upon registration to the hub:

```
{
    "apiVersion": "1.0",
    "projectId": "unique-project-identifier",
    "projectName": "Human Readable Name",
    "projectDescription": "Brief description of the prject",
    "version": "1.0.0",
    "minAttendance": 2,
    "maxAttendance": 4,
    "supportedFeatures": ["chat", "KPIs", "save-state"],
    "thumbnailBase64": "base64-encoded-image",
    "executables": {
        "windows": "project.exe",
        "macos": "project.app",
        "linux": "project.bin"
    }
}
```

#### 1.2 Registration Endpoints

- `register_project(manifest)`: Project announces itself to hub
- `unregister_project(projectId)`: Project removes itself from hub
- `get_api_version()`: Returns compatible API version

### 2. Employee Management Interface

#### 2.1 Employee Data Structure

```
{
    "userId": "uuid",
    "username": "diswork_name",
    "companyIds": ["company1", "company2"],
    "isManager": false,
    "avatar": "base64-encoded-image",
    "status": "online",  // online, away, busy, offline
    "lastSeen": "ISO-timestamp",
    "deviceHash": "unique-device-identifier",
    "publicKey": "user-public-key"
}
```

#### 2.2 Employee Endpoints

- `get_current_user()`: Returns current user information
- `update_user_status(status)`: Updates user's online status
- `join_company(companyId, invitation)`: User joins a company
- `leave_company(companyId)`: User leaves a company
- `get_friends()`: Returns friend list
- `add_friend(userId)`: Adds a user to friends
- `remove_friend(userId)`: Removes a user from friends

### 3. Company Management Interface

#### 3.1 Company Data Structure

```
{
    "companyId": "uuid",
    "companyName": "Company Name",
    "description": "Company Description",
    "logo": "base64-encoded-image",
    "departments": [
        {
            "departmentId": "uuid",
            "name": "Department Name"
        }
    ],
    "managers": ["userId1", "userId2"],
    "employees": ["userId3", "userId4", "userId5"],
    "activeProjects": ["projectId1", "projectId2"],
    "createdAt": "ISO-timestamp"
}
```

#### 3.2 Company Endpoints

- `create_company(companyData)`: Creates a new company
- `update_company(companyId, companyData)`: Updates company information
- `get_company(companyId)`: Returns company details
- `list_companies()`: Lists available companies
- `delete_company(companyId)`: Deletes a company (managers only)
- `promote_to_manager(companyId, userId)`: Promotes user to manager
- `demote_from_manager(companyId, userId)`: Demotes user from manager
- `send_company_memo(companyId, message)`: Broadcasts message to all company members

### 4. Meeting Management Interface

#### 4.1 Meeting Data Structure

```
{
    "meetingId": "uuid",
    "projectId": "project-identifier",
    "companyId": "company-identifier",
    "name": "Meeting Name",
    "status": "lobby", // lobby, in-progress, completed
    "hostId": "host-user-id",
    "attendance": [
        {
            "userId": "user-id",
            "status": "ready" // ready, not-ready, busy, offline
        }
    ],
    "meetingTranscript": {
        // Project-specific state, opaque to hub
    },
    "startedAt": "ISO-timestamp",
    "lastActivity": "ISO-timestamp",
    "settings": {
        // Project-specific settings, opaque to hub
    }
}
```

#### 4.2 Meeting Endpoints

- `create_meeting(projectId, meetingData)`: Creates a new project meeting
- `join_meeting(meetingId)`: Joins an existing meeting
- `leave_meeting(meetingId)`: Leaves a meeting
- `start_meeting(meetingId)`: Starts the project (host only)
- `end_meeting(meetingId)`: Ends the project meeting
- `update_employee_status(meetingId, status)`: Updates employee's status
- `list_active_meetings(projectId, companyId)`: Lists active meeting meetings
- `update_transcript(meetingId, meetingTranscript)`: Updates meeting's
transcript (state)
- `get_transcript(meetingId)`: Gets current meeting state

### 5. Networking and Discovery Interface

#### 5.1 Discovery Endpoints

- `broadcast_presence()`: Announces hub presence on LAN
- `discover_hubs()`: Discovers other hubs on LAN
- `connect_to_hub(hubInfo)`: Connects to another hub
- `disconnect_from_hub(hubId)`: Disconnects from a hub

#### 5.2 Peer Data Structure

```
{
    "hubId": "uuid",
    "deviceName": "device-name",
    "ipAddress": "192.168.1.100",
    "port": 3000,
    "users": ["user-id-1", "user-id-2"],
    "companies": ["company-id-1"],
    "lastSeen": "ISO-timestamp"
}
```

### 6. State Synchronization Interface

- `sync_company_data(companyId, timestamp)`: Synchronizes company data
- `sync_meeting_data(meetingId, timestamp)`: Synchronizes meeting data
- `sync_KPIs(projectId, userId, KPIs)`: Synchronizes project KPIs
- `resolve_conflict(entityType, entityId, conflictData)`: Resolves data conflicts

### 7. Messaging Interface

#### 7.1 Message Data Structure

```
{
    "messageId": "uuid",
    "type": "chat", // chat, memo, direct, system
    "sender": "user-id",
    "recipients": ["user-id-1", "user-id-2"], // or companyId
    "content": "Message content",
    "timestamp": "ISO-timestamp",
    "replyTo": "original-message-id", // optional
    "attachments": [], // optional
    "metadata": {} // optional
}
```

#### 7.2 Messaging Endpoints

- `send_direct_message(recipientId, content)`: Sends a direct message
- `send_meeting_message(meetingId, content)`: Sends message to meeting workers
- `get_message_history(type, entityId, limit, offset)`: Gets message history
- `mark_as_read(messageId)`: Marks a message as read

### 8. Project Update Interface

- `check_for_updates(projectId)`: Checks if project has updates
- `get_project_metadata(projectId)`: Gets project metadata
- `update_project(projectId)`: Updates a project
- `get_installed_projects()`: Lists all installed projects
- `install_project(projectId)`: Installs a new project
- `uninstall_project(projectId)`: Uninstalls a project

### 9. Project-Specific KPI Interface

#### 9.1 KPI Data Structure

```
{
    "userId": "user-id",
    "projectId": "project-id",
    "companyId": "company-id", // optional
    "KPIs": {
        // Project-specific KPIs
        "projectsWorked": 10,
        "projectsWon": 5,
        "highScore": 1000,
        // Additional project-specific stats
    },
    "recognitions": [
        {
            "id": "recognition-id",
            "name": "Recognition Name",
            "description": "Recognition Description",
            "unlocked": true,
            "unlockedAt": "ISO-timestamp"
        }
    ],
    "lastUpdated": "ISO-timestamp"
}
```

#### 9.2 KPIs Endpoints

- `update_KPIs(projectId, KPIs)`: Updates user's project KPIs
- `get_user_KPIs(userId, projectId)`: Gets user's KPIs for a project
- `get_company_leaderboard(companyId, projectId)`: Gets company leaderboard

## Implementation Guidelines

### Project Client Implementation

A compliant project client should:

1. **Registration**: Register with the hub on startup
2. **User Info**: Accept user information from the hub
3. **Meeting Handling**: Allow joining/creating/leaving meetings
4. **State Management**: Maintain and synchronize project state
5. **KPIs**: Report project KPIs back to the hub
6. **Communication**: Implement required messaging interfaces

### Example Workflow

1. User launches the hub application
2. Hub discovers other instances on the LAN and syncs company/user data
3. User selects a project to work
4. Hub launches project client, passing user credentials
5. Project client registers with hub
6. User creates or joins a meeting via project client
7. Project notifies hub of meeting status changes
8. Other users join the meeting
9. Project updates meeting state as work progresses
10. Project reports KPIs back to hub when meeting ends

### Connection Management

1. Projects should handle connection drops gracefully
2. Implement reconnection logic with state recovery
3. Support meeting persistence across disconnections
4. Cache necessary data to function offline when needed

---

## Tech Stack

#### SSH-lite
Use an ssh like public/private key and transmission protocol for connection, authentication, and identification

#### git-lite
Use a git like distributed and asynchronous protocol for serverless state
management and conflict Solutions. Have a crytographic like ledger or git tree
as virtual representations of server state.
Git itself could be used for development, revision control and distribution of
the hub and individual project clients.

#### P2P
libp2p, webrtc, or zeroMQ are potential options for asynchronous, distributed
P2P commuunication


