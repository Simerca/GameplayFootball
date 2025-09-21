#ifndef _HPP_COMMENTARYMANAGER
#define _HPP_COMMENTARYMANAGER

#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

class Player;
class Team;
class Match;

class CommentaryManager {
public:
  CommentaryManager();
  ~CommentaryManager();

  void Initialize(Match* match);
  void Shutdown();

  // Event handlers
  void OnPass(Player* from, Player* to);
  void OnShot(Player* player, bool onTarget);
  void OnGoal(Player* scorer, Team* team);
  void OnSave(Player* keeper);
  void OnTackle(Player* tackler, Player* tackled, bool successful);
  void OnFoul(Player* fouler, Player* fouled);
  void OnMatchStart(const std::string& team1, const std::string& team2);
  void OnHalfTime(int homeScore, int awayScore);
  void OnFullTime(int homeScore, int awayScore);
  void OnPossessionChange(Team* newTeam);
  void OnCornerKick(Team* team);
  void OnFreeKick(Team* team);
  void OnThrowIn(Team* team);
  void OnYellowCard(Player* player);
  void OnRedCard(Player* player);
  void OnSubstitution(Player* playerOut, Player* playerIn);

  // Commentary control
  void SetEnabled(bool enabled) { isEnabled = enabled; }
  bool IsEnabled() const { return isEnabled; }
  void SetVolume(int volume) { commentaryVolume = volume; }
  void SetLanguage(const std::string& lang) { language = lang; }

private:
  void Speak(const std::string& text, int priority = 0);
  void SpeakAsync(const std::string& text);
  
  std::string GetPassCommentary(const std::string& from, const std::string& to, int passCount);
  std::string GetShotCommentary(const std::string& player, bool onTarget);
  std::string GetGoalCommentary(const std::string& scorer, const std::string& team, int homeScore, int awayScore);
  std::string GetSaveCommentary(const std::string& keeper);
  std::string GetTackleCommentary(const std::string& tackler, bool successful);
  std::string GetMatchTimeCommentary(int minutes);
  std::string GetExcitementPhrase();
  std::string GetTransitionPhrase();
  
  // Helper functions
  std::string GetRandomPhrase(const std::vector<std::string>& phrases);
  bool ShouldComment(float probability = 0.3f);
  int GetMatchMinute();
  
  // State tracking
  Match* currentMatch;
  std::atomic<bool> isEnabled;
  std::atomic<bool> isSpeaking;
  int commentaryVolume;
  std::string language;
  
  // Commentary cooldowns and tracking
  std::chrono::steady_clock::time_point lastCommentTime;
  std::chrono::milliseconds minCommentInterval;
  int consecutivePasses;
  std::string lastFromPlayer;
  std::string lastToPlayer;
  int passStreak;
  Team* dominantTeam;
  int dominantPossessionCount;
  
  // Random number generation
  std::mt19937 rng;
  std::uniform_real_distribution<float> dist;
  
  // Thread safety
  std::mutex speakMutex;
  
  // Commentary templates
  std::vector<std::string> passTemplates;
  std::vector<std::string> shortPassTemplates;
  std::vector<std::string> longPassTemplates;
  std::vector<std::string> backPassTemplates;
  std::vector<std::string> throughBallTemplates;
  std::vector<std::string> shotTemplates;
  std::vector<std::string> goalTemplates;
  std::vector<std::string> saveTemplates;
  std::vector<std::string> tackleTemplates;
  std::vector<std::string> buildUpTemplates;
  std::vector<std::string> excitementPhrases;
  std::vector<std::string> transitionPhrases;
  std::vector<std::string> possessionTemplates;
  
  void LoadCommentaryTemplates();
};

#endif
