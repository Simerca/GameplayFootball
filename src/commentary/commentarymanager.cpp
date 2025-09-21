#include "commentarymanager.hpp"
#include "../onthepitch/match.hpp"
#include "../onthepitch/player/player.hpp"
#include "../onthepitch/team.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

CommentaryManager::CommentaryManager() 
  : currentMatch(nullptr), isEnabled(true), isSpeaking(false),
    commentaryVolume(200), language("en"),
    minCommentInterval(std::chrono::milliseconds(3000)),
    consecutivePasses(0), passStreak(0),
    dominantTeam(nullptr), dominantPossessionCount(0),
    rng(std::chrono::steady_clock::now().time_since_epoch().count()),
    dist(0.0f, 1.0f) {
  LoadCommentaryTemplates();
  lastCommentTime = std::chrono::steady_clock::now() - minCommentInterval;
}

CommentaryManager::~CommentaryManager() {
  Shutdown();
}

void CommentaryManager::Initialize(Match* match) {
  currentMatch = match;
  consecutivePasses = 0;
  passStreak = 0;
  dominantTeam = nullptr;
  dominantPossessionCount = 0;
}

void CommentaryManager::Shutdown() {
  isEnabled = false;
  // Wait for any ongoing speech to finish
  while (isSpeaking) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void CommentaryManager::LoadCommentaryTemplates() {
  // Pass commentary variations
  passTemplates = {
    "{from} finds {to}",
    "{from} to {to}",
    "Nice ball from {from} to {to}",
    "{from} picks out {to}",
    "Good distribution by {from}",
    "{from} with the pass",
    "Played to {to}",
    "{to} receives from {from}",
    "{from} threads it through to {to}"
  };
  
  shortPassTemplates = {
    "Quick pass to {to}",
    "Short ball to {to}",
    "Tidy play from {from}",
    "Simple pass to {to}",
    "{from} keeps it simple"
  };
  
  longPassTemplates = {
    "Long ball from {from}!",
    "Ambitious pass from {from} to {to}",
    "{from} goes long to {to}",
    "Diagonal ball from {from}",
    "Switching play, {from} to {to}"
  };
  
  backPassTemplates = {
    "Back to {to}",
    "Safety first from {from}",
    "{from} plays it back",
    "Recycling possession"
  };
  
  throughBallTemplates = {
    "Through ball from {from}!",
    "{from} splits the defense!",
    "Brilliant pass from {from} to {to}!",
    "What a ball from {from}!",
    "{to} is through!"
  };
  
  shotTemplates = {
    "{player} shoots!",
    "{player} goes for goal!",
    "Strike from {player}!",
    "{player} lets fly!",
    "{player} has a go!",
    "Effort from {player}!",
    "{player} tries his luck!"
  };
  
  goalTemplates = {
    "GOAL! {scorer} scores!",
    "IT'S IN! {scorer}!",
    "{scorer} finds the net!",
    "BRILLIANT GOAL by {scorer}!",
    "What a finish from {scorer}!",
    "{team} take the lead through {scorer}!",
    "{scorer} makes it {home} - {away}!"
  };
  
  saveTemplates = {
    "Great save by {keeper}!",
    "{keeper} denies the shot!",
    "Brilliant stop from {keeper}!",
    "{keeper} keeps it out!",
    "What a save!",
    "{keeper} to the rescue!"
  };
  
  tackleTemplates = {
    "Strong tackle from {tackler}",
    "{tackler} wins the ball",
    "Good defensive work from {tackler}",
    "{tackler} with the challenge",
    "Ball won by {tackler}",
    "Excellent tackling from {tackler}"
  };
  
  buildUpTemplates = {
    "Building from the back",
    "Patient build-up play",
    "Working the ball forward",
    "Looking for an opening",
    "Probing for space",
    "Keeping possession"
  };
  
  excitementPhrases = {
    "This is exciting!",
    "End to end stuff!",
    "What a game!",
    "Brilliant play!",
    "Fantastic football!",
    "The crowd are on their feet!"
  };
  
  transitionPhrases = {
    "Here they come",
    "On the counter",
    "Breaking forward",
    "Quick transition",
    "They're away",
    "Pushing forward now"
  };
  
  possessionTemplates = {
    "{team} dominating possession",
    "{team} in control",
    "{team} keeping the ball well",
    "Good spell for {team}",
    "{team} dictating the tempo"
  };
}

void CommentaryManager::OnPass(Player* from, Player* to) {
  if (!isEnabled || !from || !to) return;
  
  // Check cooldown
  auto now = std::chrono::steady_clock::now();
  if (now - lastCommentTime < minCommentInterval) {
    return;
  }
  
  std::string fromName = from->GetPlayerData()->GetLastName();
  std::string toName = to->GetPlayerData()->GetLastName();
  
  // Track pass patterns
  if (fromName == lastToPlayer) {
    consecutivePasses++;
  } else {
    consecutivePasses = 1;
  }
  
  // Check if it's the same pass combination (to avoid repetition)
  if (fromName == lastFromPlayer && toName == lastToPlayer) {
    return; // Skip commenting on the same pass combination
  }
  
  // Determine pass type and select appropriate commentary
  std::string commentary;
  
  // Comment on pass streaks
  if (consecutivePasses >= 5 && ShouldComment(0.7f)) {
    commentary = GetRandomPhrase(buildUpTemplates);
    if (consecutivePasses >= 8) {
      commentary += "... Excellent possession play!";
    }
  }
  // Regular pass commentary with variation
  else if (ShouldComment(0.25f)) { // Only comment on 25% of passes
    if (consecutivePasses >= 3) {
      // One-two or triangle passing
      commentary = "Quick one-two between " + fromName + " and " + toName;
    } else {
      // Random pass template
      commentary = GetPassCommentary(fromName, toName, consecutivePasses);
    }
  }
  
  if (!commentary.empty()) {
    Speak(commentary);
    lastCommentTime = now;
    lastFromPlayer = fromName;
    lastToPlayer = toName;
  }
}

void CommentaryManager::OnShot(Player* player, bool onTarget) {
  if (!isEnabled || !player) return;
  
  std::string playerName = player->GetPlayerData()->GetLastName();
  std::string commentary = GetShotCommentary(playerName, onTarget);
  
  if (onTarget) {
    commentary += "... It's on target!";
  } else {
    commentary += "... Wide!";
  }
  
  Speak(commentary, 2); // Higher priority for shots
}

void CommentaryManager::OnGoal(Player* scorer, Team* team) {
  if (!isEnabled || !scorer || !team) return;
  
  std::string scorerName = scorer->GetPlayerData()->GetLastName();
  std::string teamName = team->GetTeamData()->GetName();
  
  // This would need match score tracking
  std::string commentary = "GOOOOOAL! " + scorerName + " scores for " + teamName + "!";
  
  Speak(commentary, 3); // Highest priority for goals
}

void CommentaryManager::OnSave(Player* keeper) {
  if (!isEnabled || !keeper) return;
  
  std::string keeperName = keeper->GetPlayerData()->GetLastName();
  std::string commentary = GetSaveCommentary(keeperName);
  
  Speak(commentary, 2);
}

void CommentaryManager::OnTackle(Player* tackler, Player* tackled, bool successful) {
  if (!isEnabled || !tackler) return;
  
  if (!ShouldComment(0.2f)) return; // Only comment on 20% of tackles
  
  std::string tacklerName = tackler->GetPlayerData()->GetLastName();
  std::string commentary = GetTackleCommentary(tacklerName, successful);
  
  if (!successful) {
    commentary = "Foul by " + tacklerName + "!";
  }
  
  Speak(commentary, 1);
}

void CommentaryManager::OnMatchStart(const std::string& team1, const std::string& team2) {
  if (!isEnabled) return;
  
  std::string commentary = "Welcome to today's match between " + team1 + " and " + team2 + "! The referee blows his whistle and we're underway!";
  Speak(commentary, 3);
}

void CommentaryManager::OnHalfTime(int homeScore, int awayScore) {
  if (!isEnabled) return;
  
  std::stringstream ss;
  ss << "That's half time! The score is " << homeScore << " - " << awayScore;
  Speak(ss.str(), 3);
}

void CommentaryManager::OnFullTime(int homeScore, int awayScore) {
  if (!isEnabled) return;
  
  std::stringstream ss;
  ss << "Full time! Final score: " << homeScore << " - " << awayScore;
  if (homeScore > awayScore) {
    ss << ". Victory for the home side!";
  } else if (awayScore > homeScore) {
    ss << ". The away team takes all three points!";
  } else {
    ss << ". It's a draw!";
  }
  Speak(ss.str(), 3);
}

std::string CommentaryManager::GetPassCommentary(const std::string& from, const std::string& to, int passCount) {
  std::vector<std::string>* templates = &passTemplates;
  
  // Select template based on context
  if (passCount >= 3) {
    templates = &buildUpTemplates;
  }
  
  std::string commentary = GetRandomPhrase(*templates);
  
  // Replace placeholders
  size_t pos = commentary.find("{from}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 6, from);
  }
  
  pos = commentary.find("{to}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 4, to);
  }
  
  return commentary;
}

std::string CommentaryManager::GetShotCommentary(const std::string& player, bool onTarget) {
  std::string commentary = GetRandomPhrase(shotTemplates);
  
  size_t pos = commentary.find("{player}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 8, player);
  }
  
  return commentary;
}

std::string CommentaryManager::GetGoalCommentary(const std::string& scorer, const std::string& team, int homeScore, int awayScore) {
  std::string commentary = GetRandomPhrase(goalTemplates);
  
  // Replace placeholders
  size_t pos = commentary.find("{scorer}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 8, scorer);
  }
  
  pos = commentary.find("{team}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 6, team);
  }
  
  pos = commentary.find("{home}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 6, std::to_string(homeScore));
  }
  
  pos = commentary.find("{away}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 6, std::to_string(awayScore));
  }
  
  return commentary;
}

std::string CommentaryManager::GetSaveCommentary(const std::string& keeper) {
  std::string commentary = GetRandomPhrase(saveTemplates);
  
  size_t pos = commentary.find("{keeper}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 8, keeper);
  }
  
  return commentary;
}

std::string CommentaryManager::GetTackleCommentary(const std::string& tackler, bool successful) {
  std::string commentary = GetRandomPhrase(tackleTemplates);
  
  size_t pos = commentary.find("{tackler}");
  if (pos != std::string::npos) {
    commentary.replace(pos, 9, tackler);
  }
  
  return commentary;
}

std::string CommentaryManager::GetRandomPhrase(const std::vector<std::string>& phrases) {
  if (phrases.empty()) return "";
  
  std::uniform_int_distribution<size_t> indexDist(0, phrases.size() - 1);
  return phrases[indexDist(rng)];
}

bool CommentaryManager::ShouldComment(float probability) {
  return dist(rng) < probability;
}

void CommentaryManager::Speak(const std::string& text, int priority) {
  if (!isEnabled || text.empty()) return;
  
  // Use detached thread for non-blocking speech
  std::thread([this, text]() {
    std::lock_guard<std::mutex> lock(speakMutex);
    isSpeaking = true;
    
    std::string command = "say -r " + std::to_string(commentaryVolume) + " \"" + text + "\" 2>/dev/null";
    std::system(command.c_str());
    
    isSpeaking = false;
  }).detach();
  
  std::cout << "[COMMENTARY] " << text << std::endl;
}

void CommentaryManager::SpeakAsync(const std::string& text) {
  Speak(text, 0);
}

void CommentaryManager::OnPossessionChange(Team* newTeam) {
  // Implement possession change commentary
}

void CommentaryManager::OnCornerKick(Team* team) {
  if (!isEnabled || !team) return;
  std::string teamName = team->GetTeamData()->GetName();
  Speak("Corner kick for " + teamName, 1);
}

void CommentaryManager::OnFreeKick(Team* team) {
  if (!isEnabled || !team) return;
  std::string teamName = team->GetTeamData()->GetName();
  Speak("Free kick awarded to " + teamName, 1);
}

void CommentaryManager::OnThrowIn(Team* team) {
  // Too frequent, usually don't comment
}

void CommentaryManager::OnFoul(Player* fouler, Player* fouled) {
  if (!isEnabled || !fouler) return;
  std::string foulerName = fouler->GetPlayerData()->GetLastName();
  Speak("Foul by " + foulerName, 1);
}

void CommentaryManager::OnYellowCard(Player* player) {
  if (!isEnabled || !player) return;
  std::string playerName = player->GetPlayerData()->GetLastName();
  Speak("Yellow card for " + playerName + "!", 2);
}

void CommentaryManager::OnRedCard(Player* player) {
  if (!isEnabled || !player) return;
  std::string playerName = player->GetPlayerData()->GetLastName();
  Speak("RED CARD! " + playerName + " is sent off!", 3);
}

void CommentaryManager::OnSubstitution(Player* playerOut, Player* playerIn) {
  if (!isEnabled || !playerOut || !playerIn) return;
  std::string outName = playerOut->GetPlayerData()->GetLastName();
  std::string inName = playerIn->GetPlayerData()->GetLastName();
  Speak("Substitution: " + inName + " replaces " + outName, 1);
}

int CommentaryManager::GetMatchMinute() {
  // This would need to get the actual match time from the Match object
  return 0;
}
