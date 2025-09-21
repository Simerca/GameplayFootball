#ifndef _HPP_MATCHCOMMENTARY
#define _HPP_MATCHCOMMENTARY

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>

// Forward declarations
namespace Blunted {
  class Match;
  class Player;
  class Team;
}

class MatchCommentary {
public:
  MatchCommentary();
  ~MatchCommentary();

  // Initialize the commentary system
  void Initialize(Blunted::Match* match);
  void Shutdown();

  // Event triggers for commentary
  void OnMatchStart(const std::string& team1, const std::string& team2);
  void OnGoal(const std::string& scorerName, const std::string& teamName, int homeScore, int awayScore);
  void OnPass(const std::string& fromPlayer, const std::string& toPlayer, const std::string& teamName, bool isLongPass = false);
  void OnShot(const std::string& playerName, const std::string& teamName, bool onTarget);
  void OnSave(const std::string& keeperName, const std::string& shooterName);
  void OnFoul(const std::string& foulingPlayer, const std::string& fouledPlayer, bool isYellowCard = false, bool isRedCard = false);
  void OnCornerKick(const std::string& teamName);
  void OnThrowIn(const std::string& teamName);
  void OnOffside(const std::string& playerName);
  void OnSubstitution(const std::string& playerIn, const std::string& playerOut, const std::string& teamName);
  void OnHalfTime(int homeScore, int awayScore);
  void OnFullTime(int homeScore, int awayScore);
  void OnPossessionChange(const std::string& newTeam);
  void OnDribble(const std::string& playerName, bool successful);
  void OnTackle(const std::string& tacklingPlayer, const std::string& tackledPlayer, bool successful);
  void OnInterception(const std::string& playerName, const std::string& teamName);
  void OnClearance(const std::string& playerName);
  
  // Time-based commentary
  void OnMinuteMark(int minute);
  void OnExcitingPlay();
  
  // Dynamic commentary based on match state
  void UpdateMatchContext(int minute, int homeScore, int awayScore, float homePossession);

private:
  // Text-to-speech worker thread
  void SpeechWorker();
  
  // Generate contextual commentary
  std::string GenerateCommentary(const std::string& eventType, const std::vector<std::string>& params);
  std::string GetRandomVariation(const std::vector<std::string>& variations);
  
  // Queue management
  void AddCommentary(const std::string& text, int priority = 5);
  void ClearQueue();
  
  // Python TTS integration
  void InitializeTTS();
  void ShutdownTTS();
  void Speak(const std::string& text);
  
  // Commentary variations for different events
  std::unordered_map<std::string, std::vector<std::string>> commentaryTemplates;
  
  // Commentary queue
  struct CommentaryItem {
    std::string text;
    int priority;
    bool operator<(const CommentaryItem& other) const {
      return priority < other.priority; // Higher priority first
    }
  };
  
  std::priority_queue<CommentaryItem> commentaryQueue;
  std::mutex queueMutex;
  std::condition_variable queueCV;
  
  // Worker thread
  std::unique_ptr<std::thread> speechThread;
  std::atomic<bool> isRunning;
  
  // Match context
  Blunted::Match* currentMatch;
  int lastCommentaryMinute;
  int consecutivePasses;
  std::string lastPossessionTeam;
  bool isExcitingMoment;
  
  // TTS state
  void* ttsEngine; // Will hold Python TTS object
  bool ttsInitialized;
  
  // Commentary cooldowns to avoid spam
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastCommentaryTime;
  std::chrono::milliseconds commentaryCooldown;
  
  // Player name database (can be loaded from match data)
  std::unordered_map<int, std::string> playerNames;
  
  void LoadCommentaryTemplates();
  bool ShouldComment(const std::string& eventType);
};

#endif
