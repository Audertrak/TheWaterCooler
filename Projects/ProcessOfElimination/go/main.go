package logic

import (
	"fmt"
	"math/rand"
	"sort"
	"strings"
	"time"
)

// type enum that represents the current state of the meeting
type MeetingState int

// possible meeting states
const (
	WAITING MeetingState = iota
	PLAYING
	WON
	LOST
)

// func to return current state of the meeting
func (t MeetingState) String() string {
	switch t {
	case WAITING:
		return "WAITING"
	case PLAYING:
		return "PLAYING"
	case WON:
		return "WON"
	case LOST:
		return "LOST"
	default:
		return fmt.Sprintf("Unknown(%d)", int(t))
	}
}

// the 'meeting' is the 'top level' data structure that
type PoEMeeting struct {
	Problem            string
	WordDisplay        string
	ProposedSolutions  []string
	IncorrectSolutions int
	MaxIncorrect       int
	Status             MeetingState
}

type Transcript struct {
	problems           []string
	problem            string
	proposedSolutions  map[rune]bool
	incorrectSolutions int
	timeLimit          int
	state              MeetingState
}
