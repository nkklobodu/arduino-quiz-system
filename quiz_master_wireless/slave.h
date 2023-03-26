// Defines a class called "team" that encapsulates a team's attributes and behavior
class slave {
  public:
    int id;       //slave ID
    String name;  //slave name
    int score;    //slave score
    // uint8_t pipe;  //slave address

    // constructor for the team class, takes in the team ID, name, and radio pipe number
    slave (uint8_t id_, String name_ /*, uint8_t pipe_*/) {
      id = id_;
      name = name_;
      // pipe = pipe_;
      score = 0;  //initialize the team's score to zero
    }

    // adds the given amount of marks to the team's score
    void add(int marks) {
      score += marks;
    }

};
