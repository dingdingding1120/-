package raft

//
// this is an outline of the API that raft must expose to
// the service (or tester). see comments below for
// each of these functions for more details.
//
// rf = Make(...)
//   create a new Raft server.
// rf.Start(command interface{}) (index, term, isleader)
//   start agreement on a new log entry
// rf.GetState() (term, isLeader)
//   ask a Raft for its current term, and whether it thinks it is leader
// ApplyMsg
//   each time a new entry is committed to the log, each Raft peer
//   should send an ApplyMsg to the service (or tester)
//   in the same server.
//

import "sync"
import "labrpc"
import "time"
import "math/rand"
import "fmt"
import "encoding/gob"
import "bytes"

// import "bytes"
// import "encoding/gob"

type LogEntry struct {
    Term int
    Cmd interface{}
    LogIndex int
}

//
// as each Raft peer becomes aware that successive log entries are
// committed, the peer should send an ApplyMsg to the service (or
// tester) on the same server, via the applyCh passed to Make().
//
type ApplyMsg struct {
    Index       int
    Command     interface{}
    UseSnapshot bool   // ignore for lab2; only used in lab3
    Snapshot    []byte // ignore for lab2; only used in lab3
}

const (
       Follower = 0
       Candidate = 1
       Leader = 2
       )

//
// A Go object implementing a single Raft peer.
//
type Raft struct {
    mu        sync.Mutex          // Lock to protect shared access to this peer's state
    peers     []*labrpc.ClientEnd // RPC end points of all peers
    persister *Persister          // Object to hold this peer's persisted state
    me        int                 // this peer's index into peers[]
    
    // Your data here (2A, 2B, 2C).
    // Look at the paper's Figure 2 for a description of what
    // state a Raft server must maintain.
    
    //Peristent state on all servers
    currentTerm int
    votedFor    int
    logs         []LogEntry
    newEntry LogEntry
    
    state             int
    
    //Volatile state on all servers
    commitIndex    int
    currentLogIndex int
    lastApplied int
    
    //volatile state on leaders
    nextIndex   []int
    matchIndex  []int
    
    applyCh chan ApplyMsg
    heartbeatCh  chan int
    newLeaderCh chan int
    clientRequestCh chan int
    numVotes int
    
    //if killed, live = 0
    live bool
    
    //each round of brocastAE, sum the number of connected peers
    connectedPeers int
    commitMajor int
}

// return currentTerm and whether this server
// believes it is the leader.
func (rf *Raft) GetState() (int, bool) {
    
    var term int
    var isLeader bool
    term = rf.currentTerm
    isLeader = rf.state == Leader
    // Your code here (2A).
    return term, isLeader
    }
    
    //
    // save Raft's persistent state to stable storage,
    // where it can later be retrieved after a crash and restart.
    // see paper's Figure 2 for a description of what should be persistent.
    //
    func (rf *Raft) persist() {
        // Your code here (2C).
        // Example:
        // w := new(bytes.Buffer)
        // e := gob.NewEncoder(w)
        // e.Encode(rf.xxx)
        // e.Encode(rf.yyy)
        // data := w.Bytes()
        // rf.persister.SaveRaftState(data)
        
        w := new(bytes.Buffer)
        e := gob.NewEncoder(w)
        e.Encode(rf.currentTerm)
        e.Encode(rf.votedFor)
        for _, log := range rf.logs {
            e.Encode(log)
        }
        data := w.Bytes()
        rf.persister.SaveRaftState(data)
    }
    
    //
    // restore previously persisted state.
    //
    func (rf *Raft) readPersist(data []byte) {
        // Your code here (2C).
        // Example:
        // r := bytes.NewBuffer(data)
        // d := gob.NewDecoder(r)
        // d.Decode(&rf.xxx)
        // d.Decode(&rf.yyy)
        //fmt.Printf("readPersist: len(data) is: %v\n", len(data))
        if data == nil || len(data) < 1 { // bootstrap without any state?
            return
        }
        r := bytes.NewBuffer(data)
        d := gob.NewDecoder(r)
        d.Decode(&rf.currentTerm)
        d.Decode(&rf.votedFor)
        log := LogEntry{}
        err := d.Decode(&log)
        for err == nil {
            if log != (LogEntry{-1, -1, -1}) {
                rf.logs = append(rf.logs, log)
            }
            err = d.Decode(&log)
        }
    }
    
    
    
    
    //
    // example RequestVote RPC arguments structure.
    // field names must start with capital letters!
    //
    type RequestVoteArgs struct {
        // Your data here (2A, 2B).
        Term int
        CandidateID int
        LastLogIndex int
        LastLogTerm int
    }
    
    //
    // example RequestVote RPC reply structure.
    // field names must start with capital letters!
    //
    type RequestVoteReply struct {
        // Your data here (2A).
        Term int
        VoteGranted bool
    }
    
    type AppendEntriesArgs struct {
        Term int
        LeaderID int
        PrevLogIndex int
        PrevLogTerm int
        Entries []LogEntry
        LeaderCommit int
    }
    
    type AppendEntriesReply struct {
        Term int
        Success bool
    }
    
    //
    // example RequestVote RPC handler.
    //
    func (rf *Raft) RequestVote(args *RequestVoteArgs, reply *RequestVoteReply) {
        // Your code here (2A, 2B).
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in RequestVote\n\n", rf.me)
        defer rf.mu.Unlock()
        //defer fmt.Printf("\nserver %v locked in RequestVote\n\n", rf.me)
        
        if args.Term > rf.currentTerm {
            rf.currentTerm = args.Term
            rf.state = Follower
            rf.votedFor = -1
        }
        //DPrintf("Server %v received sendRequestVote from server %v\n", rf.me, args.CandidateID)
        if (rf.votedFor == -1 || rf.votedFor == args.CandidateID) {
            myLogLength := len(rf.logs) - 1
            if (args.LastLogTerm > rf.logs[myLogLength].Term) || (args.LastLogTerm == rf.logs[myLogLength].Term && args.LastLogIndex >= rf.logs[myLogLength].LogIndex) {
                rf.votedFor = args.CandidateID
                reply.Term = rf.currentTerm
                reply.VoteGranted = true
            } else {
                reply.Term = rf.currentTerm
                reply.VoteGranted = false
            }
        } else {
            reply.Term = rf.currentTerm
            reply.VoteGranted = false
        }
        rf.persist()
    }
    
    func (rf *Raft) AppendEntries(args *AppendEntriesArgs, reply *AppendEntriesReply) {
        fmt.Printf("Server %v(Term %v) received entries %+v from server %v(Term %v)\n", rf.me, rf.currentTerm, args.Entries, args.LeaderID, args.Term)
        //fmt.Printf("Server %v(Term %v) received sendAppendEntries from server %v(Term %v)\n", rf.me, rf.currentTerm, args.LeaderID, args.Term)
        //fmt.Printf("args.PrevLogIndex: %v, args.PrevLogTerm: %v\n", args.PrevLogIndex, args.PrevLogTerm)
        //fmt.Printf("args.Entries:\n")
        //for _, entry := range args.Entries {
        //  fmt.Printf("{entry.LogIndex:%v, entry.Term:%v, entry.Cmd:%v}\n", entry.LogIndex, entry.Term, entry.Cmd)
        //}
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in AppendEntries\n\n", rf.me)
        defer rf.mu.Unlock()
        //defer fmt.Printf("\nserver %v unlocked in AppendEntries\n\n", rf.me)
        
        if args.Term >= rf.currentTerm {
            rf.currentTerm = args.Term
            rf.state = Follower
            rf.votedFor = args.LeaderID
            rf.numVotes = 0
            rf.persist()
            
            go func() {
                rf.heartbeatCh <- 1
            }()
            
            if len(rf.logs) <=  args.PrevLogIndex {
                //means rf doesn't contain an entry at PrevLogIndex
                reply.Term = rf.currentTerm
                //fmt.Printf("I'm here1\n")
                //fmt.Printf("len(rf.logs) is: %v, args.PrevLogIndex is: %v\n", len(rf.logs), args.PrevLogIndex)
                reply.Success = false
                return
            } else if (rf.logs[args.PrevLogIndex].Term != args.PrevLogTerm) {
                //means rf contain an entry at PrevLogIndex, but term is not matching
                //fmt.Printf("I'm here2\n")
                reply.Term = rf.currentTerm
                reply.Success = false
                return
            }
            
            if len(args.Entries) == 0 {
                //It's a heartbeat, containing no cmd
                reply.Term = rf.currentTerm
                reply.Success = true
                if args.LeaderCommit > rf.commitIndex {
                    if args.LeaderCommit > len(rf.logs) - 1 {
                        rf.commitIndex = len(rf.logs) - 1
                    } else {
                        rf.commitIndex = args.LeaderCommit
                    }
                }
                return
            }
            //fmt.Printf("I'm here\n")
            //fmt.Printf("server %v: Leader commitIndex is %v, my commitIndex is %v\n", rf.me, args.LeaderCommit, rf.commitIndex)
            //fmt.Printf("args.Entries.logIndex is: %v, cmd is: %v\n", args.Entries.LogIndex, args.Entries.Cmd)
            
            if args.PrevLogIndex == 0 {
                //this is the first log entry
                for _, entry := range args.Entries {
                    rf.logs = append(rf.logs, entry)
                }
                reply.Term = rf.currentTerm
                reply.Success = true
                //fmt.Printf("111server %v: newEntry's index is: %v\n", rf.me, rf.logs[args.PrevLogIndex + 1].LogIndex)
                
                if args.LeaderCommit > rf.commitIndex {
                    rf.commitIndex = args.LeaderCommit
                }
                rf.persist()
                
                return
            }
            
            if len(rf.logs) <=  args.PrevLogIndex {
                //means rf doesn't contain an entry at PrevLogIndex
                reply.Term = rf.currentTerm
                //fmt.Printf("I'm here\n")
                //fmt.Printf("len(rf.logs) is: %v, args.PrevLogIndex is: %v\n", len(rf.logs), args.PrevLogIndex)
                reply.Success = false
            } else if (rf.logs[args.PrevLogIndex].Term != args.PrevLogTerm) {
                //means rf contain an entry at PrevLogIndex, but term is not matching
                reply.Term = rf.currentTerm
                reply.Success = false
            } else {
                if len(rf.logs) == args.PrevLogIndex + 1{
                    //the latest entry is matching PrevLog
                    for _, entry := range args.Entries {
                        rf.logs = append(rf.logs, entry)
                    }
                    reply.Term = rf.currentTerm
                    reply.Success = true
                    //fmt.Printf("222server %v: newEntry's index is: %v\n", rf.me, rf.logs[args.PrevLogIndex + 1].LogIndex)
                    if args.LeaderCommit > rf.commitIndex {
                        rf.commitIndex = args.LeaderCommit
                    }
                } else if rf.logs[args.PrevLogIndex + 1].Term == args.Term {
                    //contains the new entry already
                    rf.logs = rf.logs[:args.PrevLogIndex + 1]
                    for _, entry := range args.Entries {
                        rf.logs = append(rf.logs, entry)
                    }
                    reply.Term = rf.currentTerm
                    reply.Success = true
                    //fmt.Printf("333server %v: newEntry's index is: %v\n", rf.me, rf.logs[args.PrevLogIndex + 1].LogIndex)
                    
                    if args.LeaderCommit > rf.commitIndex {
                        rf.commitIndex = args.LeaderCommit
                    }
                    
                } else {
                    //contains entry at newEntry.index, but term is not matching
                    rf.logs = rf.logs[:args.PrevLogIndex + 1]
                    for _, entry := range args.Entries {
                        rf.logs = append(rf.logs, entry)
                    }
                    reply.Term = rf.currentTerm
                    reply.Success = true
                    //fmt.Printf("444server %v: newEntry's index is: %v\n", rf.me, rf.logs[args.PrevLogIndex + 1].LogIndex)
                    
                    if args.LeaderCommit > rf.commitIndex {
                        rf.commitIndex = args.LeaderCommit
                    }
                }
                rf.persist()
            }
            
            
        } else {
            reply.Term = rf.currentTerm
            reply.Success = false
        }
    }
    
    
    
    //
    // example code to send a RequestVote RPC to a server.
    // server is the index of the target server in rf.peers[].
    // expects RPC arguments in args.
    // fills in *reply with RPC reply, so caller should
    // pass &reply.
    // the types of the args and reply passed to Call() must be
    // the same as the types of the arguments declared in the
    // handler function (including whether they are pointers).
    //
    // The labrpc package simulates a lossy network, in which servers
    // may be unreachable, and in which requests and replies may be lost.
    // Call() sends a request and waits for a reply. If a reply arrives
    // within a timeout interval, Call() returns true; otherwise
    // Call() returns false. Thus Call() may not return for a while.
    // A false return can be caused by a dead server, a live server that
    // can't be reached, a lost request, or a lost reply.
    //
    // Call() is guaranteed to return (perhaps after a delay) *except* if the
    // handler function on the server side does not return.  Thus there
    // is no need to implement your own timeouts around Call().
    //
    // look at the comments in ../labrpc/labrpc.go for more details.
    //
    // if you're having trouble getting RPC to work, check that you've
    // capitalized all field names in structs passed over RPC, and
    // that the caller passes the address of the reply struct with &, not
    // the struct itself.
    //
    func (rf *Raft) sendRequestVote(server int, args *RequestVoteArgs, reply *RequestVoteReply) bool {
        //DPrintf("Server %v sending sendRequestVote to server %v\n", rf.me, server)
        ok := rf.peers[server].Call("Raft.RequestVote", args, reply)
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in sendRequestVote\n\n", rf.me)
        defer rf.mu.Unlock()
        //defer fmt.Printf("\nserver %v unlocked in sendRequestVote\n\n", rf.me)
        
        //fmt.Printf("test lock1\n")
        
        if ok == true {
            //fmt.Printf("test lock2\n")
            
            //DPrintf("Server %v received RequestVoteRPC reply from server %v, {Term:%v, VoteGranted:%v}\n", rf.me, server, reply.Term, reply.VoteGranted)
            //fmt.Printf("Server %v received RequestVoteRPC reply from server %v, {Term:%v, VoteGranted:%v}\n", rf.me, server, reply.Term, reply.VoteGranted)
            
            if args.Term != rf.currentTerm {
                //fmt.Printf("test lock4\n")
                
                return ok
            }
            
            if reply.Term > rf.currentTerm {
                //fmt.Printf("test lock5\n")
                
                //fmt.Printf("I'm here\n")
                rf.currentTerm = reply.Term
                rf.state = Follower
                rf.votedFor = -1
                rf.numVotes = 0
                rf.persist()
                
                go func() {
                    rf.heartbeatCh <- 1
                }()
                return ok
            }
            //fmt.Printf("I'm here\n")
            if reply.VoteGranted == true {
                //fmt.Printf("test lock6\n")
                
                //fmt.Printf("I'm here\n")
                rf.numVotes ++
                //DPrintf("total server number is: %v\n ", len(rf.peers))
                //DPrintf("This server's numVote is: %v\n", rf.numVotes)
                if rf.numVotes > (len(rf.peers) / 2) {
                    
                    if rf.state != Leader {
                        /*
                         //I don't know why, but in unreliable network this channel will block in small probability so I need to clear it
                         select {
                         case <- rf.newLeaderCh:
                         default:
                         }
                         */
                        //DPrintf("Server %v shoould become a leader now\n", rf.me)
                        
                        //fmt.Printf("test lock7\n")
                        
                        go func() {
                            rf.newLeaderCh <- 1
                        }()
                        //fmt.Printf("test lock8\n")
                        
                        //DPrintf("Server %v newLeaderCh <- 1\n", rf.me)
                        //fmt.Printf("Server %v newLeaderCh <- 1\n", rf.me)
                        
                    }
                }
                
            }
        } else {
            //fmt.Printf("test lock3\n")
            
            //DPrintf("Server %v receive RequestVote FAILED for peer %v\n", rf.me, server)
        }
        
        return ok
    }
    
    func (rf *Raft) sendAppendEntries(server int, args *AppendEntriesArgs, reply *AppendEntriesReply) bool {
        //DPrintf("server %v tring to send heartbeat to  server %v\n", rf.me, server)
        //fmt.Printf("server %v tring to send heartbeat/entry to  server %v\n", rf.me, server)
        ok := rf.peers[server].Call("Raft.AppendEntries", args, reply)
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in sendAppendEntries\n\n", rf.me)
        defer rf.mu.Unlock()
        //defer fmt.Printf("\nserver %v unlocked in sendAppendEntries\n\n", rf.me)
        if ok == true {
            rf.connectedPeers ++
            //("Server %v received AppendEntriesRPC reply from server %v, {Term:%v, Success:%v}\n", rf.me, server, reply.Term, reply.Success)
            fmt.Printf("connectedPeers: %v, Server %v received AppendEntriesRPC reply from server %v, {Term:%v, Success:%v}\n", rf.connectedPeers,rf.me, server, reply.Term, reply.Success)
            if reply.Term > rf.currentTerm {
                //fmt.Printf("I'm here\n")
                rf.currentTerm = reply.Term
                rf.state = Follower
                rf.votedFor = -1
                rf.numVotes = 0
                rf.persist()
                
                return true
            }
            if reply.Success {
                //if rf.newEntry != (LogEntry{}) {
                if len(args.Entries) != 0 {
                    //fmt.Printf("{Leader %v, term %v}Entry %v replicated on server %v\n", rf.me, rf.currentTerm, args.PrevLogIndex + 1, server)
                    rf.nextIndex[server] += len(args.Entries)
                    rf.matchIndex[server] = rf.nextIndex[server] - 1
                    rf.commitMajor ++
                    //fmt.Printf("commitMajor is: %v\n\n", rf.commitMajor)
                }
            } else {
                rf.nextIndex[server] --
            }
            return true
        } else {
            fmt.Printf("Server %v receive AppendEntries FAILED for peer %v\n", rf.me, server)
            return false
        }
        
    }
    
    
    func (rf *Raft) broadcastRV() {
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in broadcastRV\n\n", rf.me)
        rA := new(RequestVoteArgs)
        rA.Term = rf.currentTerm
        rA.CandidateID = rf.me
        myLogLength := len(rf.logs) - 1
        rA.LastLogIndex = rf.logs[myLogLength].LogIndex
        rA.LastLogTerm = rf.logs[myLogLength].Term
        rf.mu.Unlock()
        //fmt.Printf("\nserver %v unlocked in broadcastRV\n\n", rf.me)
        
        
        for i,_ := range rf.peers {
            if i != rf.me {
                go func(rAA *RequestVoteArgs, i int) {
                    rR := new(RequestVoteReply)
                    rf.sendRequestVote(i, rA, rR)
                }(rA, i)
            }
        }
    }
    
    func (rf *Raft) broadcastAE() {
        ////DPrintf("Leader %v broadcasting\n", rf.me)
        //fmt.Printf("\nLeader %v broadcasting, commitIndex is: %v\n\n", rf.me, rf.commitIndex)
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in broadcastAE\n\n", rf.me)
        defer rf.mu.Unlock()
        //defer fmt.Printf("\nserver %v unlocked in broadcastAE\n\n", rf.me)
        
        rf.connectedPeers  = 1
        
        //aA.Entries = LogEntry{}
        /*
         if rf.newEntry != (LogEntry{}) {
         aA.Entries = append(aA.Entries, rf.newEntry)
         fmt.Printf("rf.newEntry: {%v, %v, %v}\n", rf.newEntry.LogIndex, rf.newEntry.Term, rf.newEntry.Cmd)
         //fmt.Printf("\n")
         //fmt.Printf("server %v: in broadcastAE, aA.Entries:{%v, %v, %v}\n", rf.me, aA.Entries.Term, aA.Entries.Cmd, aA.Entries.LogIndex)
         aA.PrevLogIndex = rf.newEntry.LogIndex - 1
         //fmt.Printf("rf.newEntry.LogIndex is: %v\n\n", rf.newEntry.logIndex)
         if aA.PrevLogIndex == 0 {
         aA.PrevLogTerm = 0
         } else {
         aA.PrevLogTerm = rf.logs[rf.newEntry.LogIndex - 1].Term
         }
         }*/
        //fmt.Printf("Leader broadcasting, and aA's PrevLogIndex is:%v\n", aA.PrevLogIndex)
        for i,_ := range rf.peers {
            aA := new(AppendEntriesArgs)
            aA.Term = rf.currentTerm
            aA.LeaderID = rf.me
            aA.LeaderCommit = rf.commitIndex
            aA.PrevLogIndex = rf.nextIndex[i] - 1
            aA.PrevLogTerm = rf.logs[aA.PrevLogIndex].Term
            if i != rf.me {
                
                if len(rf.logs) - 1 >= rf.nextIndex[i] {
                    //fmt.Printf("last log index of leader: %v, server %v's nextIndex: %v\n", len(rf.logs) - 1, i, rf.nextIndex[i])
                    //Entries := rf.logs[rf.nextIndex[i]:]
                    //fmt.Printf("aA.Entries: {%v, %v, %v}\n", Entries[0].LogIndex, Entries[0].Term, Entries[0].Cmd)
                    aA.Entries = rf.logs[rf.nextIndex[i]:]
                }
                //fmt.Printf("server %v: len(aA.Entries) is: %v\n", i, len(aA.Entries))
                //fmt.Printf("Leader %v, sending AE to server %v, rf.nextIndex[i]: %v, aA.PrevLogIndex: %v\n", rf.me, i, rf.nextIndex[i], aA.PrevLogIndex)
                go func(aAA *AppendEntriesArgs, i int) {
                    aRR := new(AppendEntriesReply)
                    rf.sendAppendEntries(i, aAA, aRR)
                }(aA, i)
            }
        }
    }
    
    //
    // the service using Raft (e.g. a k/v server) wants to start
    // agreement on the next command to be appended to Raft's log. if this
    // server isn't the leader, returns false. otherwise start the
    // agreement and return immediately. there is no guarantee that this
    // command will ever be committed to the Raft log, since the leader
    // may fail or lose an election.
    //
    // the first return value is the index that the command will appear at
    // if it's ever committed. the second return value is the current
    // term. the third return value is true if this server believes it is
    // the leader.
    //
    func (rf *Raft) Start(command interface{}) (int, int, bool) {
        //fmt.Printf("here in start:1\n")
        rf.mu.Lock()
        //fmt.Printf("\nserver %v locked in Start\n\n", rf.me)
        defer rf.mu.Unlock()
        //  defer fmt.Printf("\nserver %v unlocked in Start\n\n", rf.me)
        index := -1
        term := -1
        isLeader := true
        
        // Your code here (2B).
        
        isLeader = (rf.state == Leader)
        term = rf.currentTerm
        
        //fmt.Printf("here in start:2\n")
        
        if isLeader == false {
            //fmt.Printf("In start(), server %v returns false\n", rf.me)
            //fmt.Printf("here in start:3\n")
            return index, term, isLeader
        }
        index = len(rf.logs)
        
        //fmt.Printf("here in start:4\n")
        
        
        newEntry := LogEntry{rf.currentTerm, command, index}
        fmt.Printf("Leader sending newEntry with index: %v\n", index)
        rf.newEntry = newEntry
        rf.logs = append(rf.logs, newEntry)
        rf.persist()
        fmt.Printf("Start(): {server:%v, state:%v, term:%v, cmd:%v} Received cmd from client, logs:%+v\n", rf.me, rf.state, rf.currentTerm, command, rf.logs)
        
        rf.matchIndex[rf.me] = len(rf.logs) - 1
        
        //fmt.Printf("here in start:5\n")
        
        /*
         go func ()  {
         fmt.Printf("server %v: address of clientRequestCh is: %v\n", rf.me, rf.clientRequestCh)
         rf.clientRequestCh <- 1
         fmt.Printf("asdl;ck;dkc;dc server %v: rf.clientRequestCh <- 1\n", rf.me)
         }()
         */
        
        return index, term, isLeader
    }
    
    //
    // the tester calls Kill() when a Raft instance won't
    // be needed again. you are not required to do anything
    // in Kill(), but it might be convenient to (for example)
    // turn off debug output from this instance.
    //
    func (rf *Raft) Kill() {
        // Your code here, if desired.
        rf.live = false
    }
    
    //
    // the service or tester wants to create a Raft server. the ports
    // of all the Raft servers (including this one) are in peers[]. this
    // server's port is peers[me]. all the servers' peers[] arrays
    // have the same order. persister is a place for this server to
    // save its persistent state, and also initially holds the most
    // recent saved state, if any. applyCh is a channel on which the
    // tester or service expects Raft to send ApplyMsg messages.
    // Make() must return quickly, so it should start goroutines
    // for any long-running work.
    //
    func Make(peers []*labrpc.ClientEnd, me int,
              persister *Persister, applyCh chan ApplyMsg) *Raft {
        rf := &Raft{}
        rf.peers = peers
        rf.persister = persister
        rf.me = me
        
        // Your initialization code here (2A, 2B, 2C).
        rf.currentTerm = 0
        rf.currentLogIndex = 0
        rf.applyCh = applyCh
        rf.state = Follower
        rf.votedFor = -1
        rf.commitIndex = 0
        rf.lastApplied = 0
        rf.heartbeatCh = make(chan int)
        rf.newLeaderCh = make(chan int)
        rf.clientRequestCh = make(chan int)
        rf.live = true
        rf.connectedPeers = 1
        rf.commitMajor = 1
        rf.logs = append(rf.logs, LogEntry{-1, -1, -1})
        rf.nextIndex = make([]int, len(rf.peers))
        rf.matchIndex = make([]int, len(rf.peers))
        rf.newEntry = LogEntry{}
        
        go func() {
            for rf.live {
                //fmt.Printf("server %v -> state is: %v, term is: %v\n", rf.me, rf.state, rf.currentTerm)
                //fmt.Printf("server %v -> commitIndex is: %v, lastApplied is: %v\n", rf.me, rf.commitIndex, rf.lastApplied)
                //for _, log := range rf.logs {
                //  fmt.Printf("{Index: %v, Term: %v, Cmd: %v}\n", log.LogIndex, log.Term, log.Cmd)
                //}
                //fmt.Printf("\n")
                
                //fmt.Printf("server %v, commit Index is: %v\n", rf.me, rf.commitIndex)
                if rf.commitIndex > rf.lastApplied {
                    rf.lastApplied ++
                    //fmt.Printf("{lastApplied: %v, commitIndex: %v, len(logs): %v, Index: %v} server %v commiting\n", rf.lastApplied, rf.commitIndex, len(rf.logs), rf.logs[rf.lastApplied].LogIndex, rf.me)
                    for ; rf.lastApplied <= rf.commitIndex; rf.lastApplied++ {
                        msg := ApplyMsg{}
                        msg.Index = rf.logs[rf.lastApplied].LogIndex
                        msg.Command = rf.logs[rf.lastApplied].Cmd
                        fmt.Printf(" applyCh: Server %d Term %d applyMsg:%+v logs:%+v\n", rf.me, rf.currentTerm, msg, rf.logs)
                        
                        rf.applyCh <- msg
                    }
                    rf.lastApplied --
                }
                //fmt.Printf("\n\nServer %v: {term:%v, len(logs):%v}\n\n", rf.me, rf.currentTerm, len(rf.logs))
                switch rf.state {
                case Follower:
                    select {
                    case <- rf.heartbeatCh:
                        ////DPrintf("Got hearbeat:")
                        //DPrintf("Server %v Got heartbeat\n", rf.me)
                    case <- time.After(time.Millisecond * time.Duration(rand.Intn(300) + 500))://election timeout
                        rf.state = Candidate
                    }
                    
                case Candidate:
                    rf.mu.Lock()
                    //fmt.Printf("\nserver %v locked in case Candidate\n\n", rf.me)
                    rf.currentTerm ++
                    //fmt.Printf("server %v, term++, term becomes %v\n\n", rf.me, rf.currentTerm)
                    rf.votedFor = rf.me
                    rf.numVotes = 1
                    rf.persist()
                    
                    rf.mu.Unlock()
                    //fmt.Printf("\nserver %v unlocked in case Candidate\n\n", rf.me)
                    
                    //DPrintf("server %v becomes new Candidate for new term\n ", rf.me)
                    //fmt.Printf("server %v becomes new Candidate for new term\n ", rf.me)
                    //fmt.Printf("time: %v\n", time.Now())
                    go rf.broadcastRV()
                    
                    select {
                    case <- time.After(time.Millisecond * time.Duration(rand.Intn(300) + 500)):
                        //if too few peers are connected, candidate shouldn't become a leader
                        if rf.connectedPeers <=  (len(rf.peers) / 2){
                            //DPrintf("there’s no quorum, no leader should be elected\n")
                            /*
                             rf.currentTerm = rf.currentTerm - 2
                             if rf.currentTerm < rf.logs[len(rf.logs) - 1].Term - 1 {
                             rf.currentTerm = rf.logs[len(rf.logs) - 1].Term - 1
                             }
                             fmt.Printf("server %v latest log tTrm is: %v\n", rf.me, rf.logs[len(rf.logs) - 1].Term)
                             */
                            //fmt.Printf("server %v,convert from Candidate to Follower, term becomes %v\n", rf.me, rf.currentTerm)
                            rf.state = Follower
                            rf.votedFor = -1
                            rf.connectedPeers = 1
                            rf.persist()
                            
                            break
                        }
                        //fmt.Printf("Candidate server %v times out\n", rf.me)
                    case <- rf.heartbeatCh:
                        //DPrintf("Server %v got heartbeat, convert from Candidate to Follower\n", rf.me)
                        rf.state = Follower
                        rf.votedFor = -1
                        rf.numVotes = 0
                        rf.persist()
                        
                    case <-rf.newLeaderCh:
                        //fmt.Printf("Server %v <-newLeaderCh\n", rf.me)
                        
                        rf.state = Leader
                        rf.connectedPeers = (len(rf.peers))
                        for i := 0; i < len(rf.peers); i++ {
                            rf.nextIndex[i] = len(rf.logs)
                            rf.matchIndex[i] = 0
                        }
                        //DPrintf("Server %v becomes new leader for term %v\n\n", rf.me, rf.currentTerm)
                        //fmt.Printf("Server %v becomes new leader for term %v\n\n", rf.me, rf.currentTerm)
                    }
                    
                case Leader:
                    //to prevent deadlock
                    select {
                    case <- rf.newLeaderCh:
                    default:
                    }
                    for i := 0; i < len(rf.peers); i++ {
                        //fmt.Printf("matchIndex[%v] is: %v\n", i, rf.matchIndex[i])
                        //fmt.Printf("nextIndex[%v] is %v\n", i, rf.nextIndex[i])
                    }
                    if rf.connectedPeers <=  (len(rf.peers) / 2){
                        //DPrintf("there’s no quorum, no leader should be elected\n")
                        fmt.Printf("connectedPeers is: %v, there’s no quorum, no leader should be elected\n", rf.connectedPeers)
                        rf.state = Follower
                        rf.votedFor = -1
                        rf.connectedPeers = 1
                        rf.persist()
                        
                        break
                    }
                    
                    rf.mu.Lock()
                    //fmt.Printf("\nserver %v locked in case Leader\n\n", rf.me)
                    //fmt.Printf("Leader %v, commitMajor is: %v\n", rf.me, rf.commitMajor)
                    /*
                     if rf.commitMajor > (len(rf.peers) / 2) {
                     rf.commitIndex ++
                     rf.commitMajor = 0  //to avoid multiple commit
                     }
                     */
                    
                    N := rf.commitIndex + 1
                    NFinal := rf.commitIndex
                    for ; N < len(rf.logs); N++ {
                        
                        if rf.logs[N].Term != rf.currentTerm {
                            continue
                        }
                        
                        count := 0
                        for _, matchIndex := range rf.matchIndex {
                            if matchIndex >= N {
                                count++
                            }
                        }
                        if count <= len(rf.peers)/2 {
                            break
                        }
                        NFinal = N
                    }
                    
                    
                    rf.commitIndex = NFinal
                    
                    //fmt.Printf("Leader %v's commitIndex changes to %v\n", rf.me, rf.commitIndex)
                    
                    rf.mu.Unlock()
                    //fmt.Printf("\nserver %v unlocked in case Leader\n\n", rf.me)
                    
                    
                    select {
                        /*
                         case <- rf.clientRequestCh:
                         fmt.Printf("server %v: <- rf.clientRequestCh\n", rf.me)
                         rf.commitMajor = 1
                         go rf.broadcastAE()
                         time.Sleep(105 * time.Millisecond)
                         //clear client related logs
                         //some code here
                         rf.newEntry = LogEntry{}
                         */
                    default:
                        rf.newEntry = LogEntry{}
                        go rf.broadcastAE()
                        time.Sleep(50 * time.Millisecond)
                    }
                }
                
            }
        }()
        
        
        // initialize from state persisted before a crash
        rf.readPersist(persister.ReadRaftState())
        
        
        return rf
    }
