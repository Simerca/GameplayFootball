#include "matchcommentary.hpp"
#include "../onthepitch/match.hpp"
#include "../onthepitch/player/player.hpp"
#include "../onthepitch/team.hpp"
#include <random>
#include <chrono>
#include <sstream>
#include <iostream>
#include <cstdlib>

MatchCommentary::MatchCommentary() 
  : isRunning(false), currentMatch(nullptr), lastCommentaryMinute(0),
    consecutivePasses(0), isExcitingMoment(false), ttsEngine(nullptr),
    ttsInitialized(false), commentaryCooldown(2000) {
  LoadCommentaryTemplates();
}

MatchCommentary::~MatchCommentary() {
  Shutdown();
}

void MatchCommentary::Initialize(Blunted::Match* match) {
  currentMatch = match;
  isRunning = true;
  
  // Initialize Python TTS
  InitializeTTS();
  
  // Start speech worker thread
  speechThread = std::make_unique<std::thread>(&MatchCommentary::SpeechWorker, this);
}

void MatchCommentary::Shutdown() {
  if (isRunning) {
    isRunning = false;
    queueCV.notify_all();
    if (speechThread && speechThread->joinable()) {
      speechThread->join();
    }
    ShutdownTTS();
  }
}

void MatchCommentary::InitializeTTS() {
  // For now, we'll use system TTS on macOS (say command)
  // This is a simpler approach that doesn't require Python integration
  ttsInitialized = true;
  std::cout << "Commentary system initialized (using system TTS)" << std::endl;
}

void MatchCommentary::ShutdownTTS() {
  ttsInitialized = false;
}

void MatchCommentary::Speak(const std::string& text) {
  if (!ttsInitialized) return;
  
  // Use macOS 'say' command for TTS
  // Run in background to avoid blocking
  std::string command = "say \"" + text + "\" &";
  std::system(command.c_str());
  
  // Print to console as well for debugging
  std::cout << "[Commentary] " << text << std::endl;
}

void MatchCommentary::SpeechWorker() {
  while (isRunning) {
    std::unique_lock<std::mutex> lock(queueMutex);
    queueCV.wait(lock, [this] { return !commentaryQueue.empty() || !isRunning; });
    
    if (!isRunning) break;
    
    if (!commentaryQueue.empty()) {
      CommentaryItem item = commentaryQueue.top();
      commentaryQueue.pop();
      lock.unlock();
      
      // Speak the commentary
      Speak(item.text);
      
      // Small delay between comments
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
}

void MatchCommentary::AddCommentary(const std::string& text, int priority) {
  std::lock_guard<std::mutex> lock(queueMutex);
  
  // Limit queue size to prevent overflow
  if (commentaryQueue.size() < 10) {
    commentaryQueue.push({text, priority});
    queueCV.notify_one();
  }
}

void MatchCommentary::LoadCommentaryTemplates() {
  // Goal commentary variations
  commentaryTemplates["goal"] = {
    "GOAL! {0} scores for {1}! The score is now {2} to {3}!",
    "What a strike from {0}! {1} takes the lead!",
    "{0} finds the back of the net! Brilliant finish for {1}!",
    "And it's in! {0} with a fantastic goal for {1}!",
    "GOOOAL! {0} puts it away for {1}! What a moment!"
  };
  
  // Pass commentary
  commentaryTemplates["pass"] = {
    "{0} finds {1} with a nice pass.",
    "Good ball from {0} to {1}.",
    "{0} threads it through to {1}.",
    "Clever pass from {0}, {1} receives it well.",
    "{0} to {1}, {2} maintaining possession."
  };
  
  commentaryTemplates["long_pass"] = {
    "Long ball from {0} searching for {1}!",
    "{0} launches it forward to {1}!",
    "Ambitious pass from {0} looking for {1} up field!"
  };
  
  // Shot commentary
  commentaryTemplates["shot_on_target"] = {
    "{0} with a shot! The keeper will have to deal with that!",
    "Strike from {0}! Testing the goalkeeper!",
    "{0} lets fly! Good effort from {1}!",
    "Shot on goal from {0}! {1} looking dangerous!"
  };
  
  commentaryTemplates["shot_off_target"] = {
    "{0} shoots... but it's wide!",
    "Off target from {0}. {1} will be disappointed with that.",
    "{0} blazes it over! Should have done better there.",
    "Wild effort from {0}. That's nowhere near the target."
  };
  
  // Save commentary
  commentaryTemplates["save"] = {
    "Great save by {0}! Denying {1} there!",
    "{0} with a fantastic stop! {1} can't believe it!",
    "What a save! {0} keeps it out!",
    "Brilliant goalkeeping from {0} to deny {1}!"
  };
  
  // Tackle commentary
  commentaryTemplates["tackle_success"] = {
    "Strong tackle from {0} on {1}!",
    "{0} wins the ball with a great challenge!",
    "Clean tackle from {0}, dispossessing {1}.",
    "{0} times it perfectly to win the ball from {1}!"
  };
  
  commentaryTemplates["tackle_fail"] = {
    "{0} goes in for the tackle but {1} evades it!",
    "{1} skips past the challenge from {0}!",
    "Nice skill from {1} to avoid {0}'s tackle!"
  };
  
  // Foul commentary
  commentaryTemplates["foul"] = {
    "Foul by {0} on {1}. Free kick awarded.",
    "{0} brings down {1}. The referee blows his whistle.",
    "That's a foul from {0}. {1} wins the free kick."
  };
  
  commentaryTemplates["yellow_card"] = {
    "Yellow card for {0}! That was a reckless challenge on {1}.",
    "{0} goes into the book for that foul on {1}.",
    "The referee shows {0} a yellow card. {1} was caught late there."
  };
  
  commentaryTemplates["red_card"] = {
    "RED CARD! {0} is sent off for that challenge on {1}!",
    "Oh no! {0} sees red! That's a terrible tackle on {1}!",
    "{0} has to go! Straight red card for that foul on {1}!"
  };
  
  // Match events
  commentaryTemplates["kickoff"] = {
    "And we're underway! {0} versus {1}!",
    "The match begins! {0} taking on {1} today!",
    "Kick off! Let's see what {0} and {1} have in store for us!"
  };
  
  commentaryTemplates["halftime"] = {
    "That's halftime! The score is {0} {1}, {2} {3}.",
    "The referee blows for halftime. {0} {1}, {2} {3} as we head to the break.",
    "End of the first half. {0} leading {2} by {1} to {3}."
  };
  
  commentaryTemplates["fulltime"] = {
    "That's the final whistle! {0} {1}, {2} {3}!",
    "It's all over! Final score: {0} {1}, {2} {3}!",
    "Full time! {0} wins {1} to {3} against {2}!"
  };
  
  // Possession and play
  commentaryTemplates["possession_change"] = {
    "{0} win back possession.",
    "Ball turned over to {0}.",
    "{0} regain control of the ball."
  };
  
  commentaryTemplates["dribble_success"] = {
    "Brilliant dribbling from {0}!",
    "{0} beats his man with some fancy footwork!",
    "Skillful play from {0} to get past the defender!"
  };
  
  commentaryTemplates["interception"] = {
    "Interception by {0}! {1} win it back!",
    "{0} reads it well and intercepts! Good awareness!",
    "Stolen by {0}! {1} back in possession!"
  };
}

std::string MatchCommentary::GetRandomVariation(const std::vector<std::string>& variations) {
  if (variations.empty()) return "";
  
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, variations.size() - 1);
  
  return variations[dis(gen)];
}

std::string MatchCommentary::GenerateCommentary(const std::string& eventType, const std::vector<std::string>& params) {
  auto it = commentaryTemplates.find(eventType);
  if (it == commentaryTemplates.end()) return "";
  
  std::string template_str = GetRandomVariation(it->second);
  
  // Replace placeholders with parameters
  for (size_t i = 0; i < params.size(); ++i) {
    std::string placeholder = "{" + std::to_string(i) + "}";
    size_t pos = template_str.find(placeholder);
    while (pos != std::string::npos) {
      template_str.replace(pos, placeholder.length(), params[i]);
      pos = template_str.find(placeholder, pos + params[i].length());
    }
  }
  
  return template_str;
}

bool MatchCommentary::ShouldComment(const std::string& eventType) {
  auto now = std::chrono::steady_clock::now();
  auto it = lastCommentaryTime.find(eventType);
  
  if (it != lastCommentaryTime.end()) {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
    if (elapsed < commentaryCooldown) {
      return false; // Too soon since last comment of this type
    }
  }
  
  lastCommentaryTime[eventType] = now;
  return true;
}

// Event implementations
void MatchCommentary::OnMatchStart(const std::string& team1, const std::string& team2) {
  std::string commentary = GenerateCommentary("kickoff", {team1, team2});
  AddCommentary(commentary, 10);
}

void MatchCommentary::OnGoal(const std::string& scorerName, const std::string& teamName, int homeScore, int awayScore) {
  std::string commentary = GenerateCommentary("goal", {
    scorerName, teamName, 
    std::to_string(homeScore), std::to_string(awayScore)
  });
  AddCommentary(commentary, 10); // High priority
  isExcitingMoment = true;
}

void MatchCommentary::OnPass(const std::string& fromPlayer, const std::string& toPlayer, const std::string& teamName, bool isLongPass) {
  consecutivePasses++;
  
  // Only comment on notable passes or build-up play
  if (consecutivePasses >= 5 || isLongPass || isExcitingMoment) {
    if (ShouldComment("pass")) {
      std::string eventType = isLongPass ? "long_pass" : "pass";
      std::string commentary = GenerateCommentary(eventType, {fromPlayer, toPlayer, teamName});
      AddCommentary(commentary, 3);
    }
  }
}

void MatchCommentary::OnShot(const std::string& playerName, const std::string& teamName, bool onTarget) {
  std::string eventType = onTarget ? "shot_on_target" : "shot_off_target";
  std::string commentary = GenerateCommentary(eventType, {playerName, teamName});
  AddCommentary(commentary, 7);
  isExcitingMoment = true;
}

void MatchCommentary::OnSave(const std::string& keeperName, const std::string& shooterName) {
  std::string commentary = GenerateCommentary("save", {keeperName, shooterName});
  AddCommentary(commentary, 8);
}

void MatchCommentary::OnFoul(const std::string& foulingPlayer, const std::string& fouledPlayer, bool isYellowCard, bool isRedCard) {
  std::string eventType = "foul";
  if (isRedCard) eventType = "red_card";
  else if (isYellowCard) eventType = "yellow_card";
  
  std::string commentary = GenerateCommentary(eventType, {foulingPlayer, fouledPlayer});
  int priority = isRedCard ? 10 : (isYellowCard ? 8 : 5);
  AddCommentary(commentary, priority);
}

void MatchCommentary::OnTackle(const std::string& tacklingPlayer, const std::string& tackledPlayer, bool successful) {
  if (ShouldComment("tackle")) {
    std::string eventType = successful ? "tackle_success" : "tackle_fail";
    std::string commentary = GenerateCommentary(eventType, {tacklingPlayer, tackledPlayer});
    AddCommentary(commentary, 4);
  }
}

void MatchCommentary::OnPossessionChange(const std::string& newTeam) {
  if (newTeam != lastPossessionTeam) {
    consecutivePasses = 0;
    lastPossessionTeam = newTeam;
    
    if (ShouldComment("possession") && isExcitingMoment) {
      std::string commentary = GenerateCommentary("possession_change", {newTeam});
      AddCommentary(commentary, 2);
    }
  }
}

void MatchCommentary::OnDribble(const std::string& playerName, bool successful) {
  if (successful && ShouldComment("dribble")) {
    std::string commentary = GenerateCommentary("dribble_success", {playerName});
    AddCommentary(commentary, 5);
    isExcitingMoment = true;
  }
}

void MatchCommentary::OnInterception(const std::string& playerName, const std::string& teamName) {
  std::string commentary = GenerateCommentary("interception", {playerName, teamName});
  AddCommentary(commentary, 6);
  consecutivePasses = 0;
}

void MatchCommentary::OnHalfTime(int homeScore, int awayScore) {
  // Assuming we have team names from match context
  std::string commentary = GenerateCommentary("halftime", {
    "Home Team", std::to_string(homeScore),
    "Away Team", std::to_string(awayScore)
  });
  AddCommentary(commentary, 10);
  isExcitingMoment = false;
}

void MatchCommentary::OnFullTime(int homeScore, int awayScore) {
  std::string commentary = GenerateCommentary("fulltime", {
    "Home Team", std::to_string(homeScore),
    "Away Team", std::to_string(awayScore)
  });
  AddCommentary(commentary, 10);
}

void MatchCommentary::UpdateMatchContext(int minute, int homeScore, int awayScore, float homePossession) {
  // Update excitement level based on match state
  if (minute > 80 && abs(homeScore - awayScore) <= 1) {
    isExcitingMoment = true; // Close game in final minutes
  }
  
  // Periodic match updates
  if (minute % 15 == 0 && minute != lastCommentaryMinute) {
    lastCommentaryMinute = minute;
    // Could add periodic commentary here
  }
}
