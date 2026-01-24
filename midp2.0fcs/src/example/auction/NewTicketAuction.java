/*
 * NewTicketAuction.java
 *
 * Copyright (c) 1999-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

package example.auction;

import java.util.*;
import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.lcdui.List;

public class NewTicketAuction extends MIDlet implements CommandListener {

    Display display;
    /** user interface alert component. */
    Alert splashScreenAlert; 
    // Wait for 2sec
    static final int DefaultTimeout = 2000;
                   
    Image   splashScreen;

    boolean imageLoaded;
    Band   band = new Band();
    Login login = new Login();
    TextBox     addBand;
    RmBand      rmBand = new RmBand();
    static final Command BACK_CMD = new Command("Back", Command.BACK, 1);
    static final Command SAVE_CMD = new Command("Save", Command.SCREEN, 2);
    static final Command NEXT_CMD = new Command("Next", Command.SCREEN, 2);
    static final Command SUBMIT_CMD = new Command("Submit", Command.ITEM, 2);
    static final Command AUCTION_CMD = new Command("Auction", Command.ITEM, 2);
    static final Command CANCEL_CMD = new Command("Cancel", 
                                                      Command.CANCEL, 1);
    static final Command STOP_CMD = new Command("Stop", 
                                                      Command.STOP, 1);

    List        mainMenu, cateMenu, ticketList; // bandList, ticketList;
    Ticker      ticker;
    Alert       notImpl, savedMsg, alertMsg;
    Form        band_ticket_Form, ticketForm, enterForm, bidForm, submitMsg;
    TextField   enterText;
    ChoiceGroup bandCg, auctionCg;
    StringItem auctionName;
    UpdateAlert updateAlert = new UpdateAlert();

    Gauge         submitGauge;
    Timer TimerService;
    SetMenuForm setMenu2 = new SetMenuForm();

    // Band Name and Index number of the selected band in 
    // Band Screen band_ticket_Form
    String _bandName;
    int    _bandIndex = 0;

    static final Command EXIT_CMD = new Command("Exit", 
                                                      Command.EXIT, 1);
    static final Command ABOUT_CMD = new Command("About Tickets", 
                                                      Command.HELP, 1);
    // commands for Band Screen bandForm

    static final Command SHOW_CMD = new Command("Show Auctions", 
                                                      Command.SCREEN, 1);
    static final Command ADD_CMD = new Command("Add Bands", 
                                                      Command.SCREEN, 1);
    static final Command RMV_CMD = new Command("Remove Bands",
                                                      Command.SCREEN, 2);
    static final Command SETTING_CMD = new Command("Settings", 
                                                      Command.SCREEN, 3);

  
    // commands for  Auction Screen ticketList

    static final Command SHOW_INFO_CMD = new Command("Show More Info",
                                                      Command.ITEM, 1);
    static final Command MAKE_BID_CMD = new Command("Make a Bid",
                                                      Command.ITEM, 2);
    static final Command SET_ALERT_CMD = new Command("Set an Alert",
                                                      Command.ITEM, 3);

    // command for Bid screen ticketForm
    static final Command BAND_CMD = new Command("Bands", 
                                      Command.SCREEN, 1);

    public NewTicketAuction() {
        display = Display.getDisplay(this);
    }

    public void startApp() {
        notImpl = new Alert("Sorry!!!", "Not Implemented", null, null);

        savedMsg = new Alert(null, "Your new settings have been saved!",
                             null, null);

        alertMsg = new Alert(null, "Your alerts have been saved!",
                             null, null);


        submitGauge = new Gauge("", false, 100, 0);
        submitMsg = new Form("Submitting bid...");
        submitMsg.append(submitGauge);
        submitMsg.addCommand(STOP_CMD);
        submitMsg.setCommandListener(this);


        ticker = new Ticker("");
        ticker.setString(band.toTickerString(null));

        band_ticket_Form = new Form("Welcome To Tickets");

        band_ticket_Form.addCommand(SHOW_INFO_CMD);
        band_ticket_Form.addCommand(MAKE_BID_CMD);
        band_ticket_Form.addCommand(SET_ALERT_CMD);

        band_ticket_Form.addCommand(ADD_CMD);
        band_ticket_Form.addCommand(RMV_CMD);
        band_ticket_Form.addCommand(SETTING_CMD);
        band_ticket_Form.addCommand(ABOUT_CMD);
        band_ticket_Form.addCommand(EXIT_CMD);
        band_ticket_Form.setCommandListener(this);
        band_ticket_Form.setTicker(ticker);

        // bandCg = new ChoiceGroup("Choose a Band", Choice.EXCLUSIVE);
        bandCg = new ChoiceGroup("Choose a Band", Choice.POPUP);
        band_ticket_Form.append(bandCg);
        String[] list = band.getList();
        for (int i = 0; i < list.length; i++) {
            bandCg.append(list[i], null);
        }

        String bandName = 
            band.getName(bandCg.getSelectedIndex());
        auctionCg = new ChoiceGroup(bandName + " Auctions:", 
				    Choice.EXCLUSIVE);
        auctionCg.setLayout(Item.LAYOUT_EXPAND);
        band_ticket_Form.append(auctionCg);
        reconstructList(auctionCg, band.getTicketList(bandName));
        band_ticket_Form.setItemStateListener(
              new ItemStateListener() {
        public void itemStateChanged(Item item) { 
            if (item instanceof ChoiceGroup) {
                ChoiceGroup obj = (ChoiceGroup) item;
                if (obj == bandCg) {
                    int idx = obj.getSelectedIndex();
                    String bandName = band.getName(idx);
                    auctionCg.setLabel(bandName + " Auctions:");
                    reconstructList(auctionCg, band.getTicketList(bandName));
                }
            }
        }});
        try {
            splashScreen = 
              Image.createImage("/example/auction/images/splashScreen.png");
            imageLoaded = true;
        } catch (java.io.IOException ex) {
        }

        splashScreenAlert = new Alert("Welcome to Ticket Auction", "", 
                             splashScreen, AlertType.INFO);
        splashScreenAlert.setTimeout(DefaultTimeout);

        display.setCurrent(splashScreenAlert, band_ticket_Form);

        ticketForm = new Form("dummy");
        ticketForm.append("dummy");
        ticketForm.addCommand(BACK_CMD);
        ticketForm.addCommand(BAND_CMD);
        ticketForm.addCommand(MAKE_BID_CMD);
        ticketForm.addCommand(SET_ALERT_CMD);
        ticketForm.setCommandListener(this);

        enterText = new TextField("Tell me when bid reaches:", "", 
                                  10, TextField.DECIMAL);

        enterForm = new Form("Set an Alert");
        enterForm.append(enterText);
        enterForm.addCommand(BACK_CMD);
        enterForm.addCommand(SAVE_CMD);
        enterForm.setCommandListener(this);

        addBand = new TextBox("Add Bands, Bands:", "", 100, TextField.ANY);
        addBand.addCommand(BACK_CMD);
        addBand.addCommand(SAVE_CMD);
        addBand.setCommandListener(this);
    }
    public void destroyApp(boolean unconditional) {
    }

    public void pauseApp() {
    }



    public void commandAction(Command c, Displayable s) {
          if (s instanceof Form) {
            Form obj = (Form) s;
            if (obj == band_ticket_Form) {
                int idx = auctionCg.getSelectedIndex();
                _bandIndex = idx;

                int bandIdx = bandCg.getSelectedIndex();
                String bandName = "";
                if (bandIdx >= 0) {
                    bandName =  band.getName(bandIdx);
                }
                 _bandName = bandName;

                if (c == SHOW_INFO_CMD) {
                    deleteFormItem(ticketForm);
                    ticketForm.setTitle(band.getTicketDataTitle(bandName, 
								idx));
                    band.getTicketData(ticketForm, bandName, idx);
                    display.setCurrent(ticketForm);
                } else if (c == MAKE_BID_CMD) {
                    login.setBandAttributes(bandName, _bandIndex);
                    deleteFormItem(login);
                    login.append(login.id);
                    login.id.setLabel("Enter Your ID:");
                    login.append(login.pd);
                    login.pd.setLabel("Enter PIN:");
                    login.append(login.curBid);
                    login.append(login.incBid);
                    login.append(login.minBid);
                    login.append(login.bidText);
                    login.id.setString(null);
                    login.pd.setString(null);
                    login.bidText.setString(null);
                    band.getTicketBidTitle(login, 
                                               _bandName, _bandIndex);
                    display.setCurrent(login);
                } else if (c == SET_ALERT_CMD) {
                    display.setCurrent(enterForm);
                } else if (c == ADD_CMD) {
                    display.setCurrent(addBand);
                } else if (c == RMV_CMD) {
                    display.setCurrent(rmBand);
                } else if (c == SETTING_CMD) {
                    display.setCurrent(setMenu2);
                } else if (c == ABOUT_CMD) {
                    example.About.showAbout(display);
                } else if (c == EXIT_CMD) {
                    notifyDestroyed();
                }
            } else if (obj == ticketForm) {
                if (c == BACK_CMD) {
                  display.setCurrent(band_ticket_Form);
                } else if (c == BAND_CMD) {
                    display.setCurrent(band_ticket_Form);
                } else if (c == MAKE_BID_CMD) {
                    login.setBandAttributes(band.getName(
					    bandCg.getSelectedIndex()),
					    _bandIndex);
                    deleteFormItem(login);
                    login.append(login.id);
                    login.id.setLabel("Enter Auction ID:");
                    login.append(login.pd);
                    login.pd.setLabel("Enter PIN:");
                    login.append(login.curBid);
                    login.append(login.incBid);
                    login.append(login.minBid);
                    login.append(login.bidText);
                    login.id.setString(null);
                    login.pd.setString(null);
                    login.bidText.setString(null);
                    band.getTicketBidTitle(login, 
                                               _bandName, _bandIndex);
                    display.setCurrent(login);
                } else if (c == SET_ALERT_CMD) {
                    deleteFormItem(enterForm);
             
                    int selectedBand = bandCg.getSelectedIndex();
                    String ticketID = 
                        band.getTicketID(band.getName(selectedBand),
					 login._bandIndex);
                    enterForm.append(new StringItem("Auction:", 
				     band.getName(selectedBand)+ " " +
						    ticketID));
                    String curBid = 
			band.getCurrentBid(band.getName(selectedBand),
					   login._bandIndex);
                    enterForm.append(new StringItem("Current Bid:",  
						    "$"+ curBid));
                    enterForm.append(enterText);
                    display.setCurrent(enterForm);
                }

            } else if (obj == enterForm) {
                 if (c == BACK_CMD) {
                    enterText.setString(null);
                    display.setCurrent(band_ticket_Form);
                } else if (c == SAVE_CMD) {
                    updateAlert.set(band.getName(bandCg.getSelectedIndex()), 
                                    enterText.getString());
                    display.setCurrent(alertMsg, band_ticket_Form);
                }
            } else if (obj == submitMsg) {
                if (c == STOP_CMD) {
                    TimerService.cancel();
                    display.setCurrent(login.confirm);
                }
            }
        } else if (s instanceof TextBox) {
            TextBox obj = (TextBox) s;
            if (obj == addBand) {
                if (c == BACK_CMD) {
                  // display.setCurrent(setMenu);
                  display.setCurrent(band_ticket_Form);
                } else if (c == SAVE_CMD) {
                    updateBandList(addBand.getString());
                    display.setCurrent(savedMsg, band_ticket_Form);
                }
            }
        }
    }

    class Login extends Form implements CommandListener {
        TextField id = new TextField("", "", 10, TextField.ANY);
        TextField pd = new TextField("", "", 10, 
                                     (TextField.NUMERIC|TextField.PASSWORD));
        TextField bidText = new TextField("Enter Bid:", "", 
					  10, TextField.DECIMAL);
        StringItem submitButton = new StringItem("", "Submit", Item.BUTTON);
        StringItem auctionHyperlink = new StringItem("Auction:", "", 
						     Item.HYPERLINK);

        StringItem curBid = new StringItem("Current Bid:", "");
        StringItem incBid = new StringItem("Increment:", "");
        StringItem minBid = new StringItem("Minimum Bid:", "");
        Form confirm = new Form("Confirm Bid");
        Form notice = new Form("Bid Received");

        Form auctionForm = new Form("dummy");

        Alert loginAlert = 
            new Alert("Alert", 
                      "Your must enter your ID and password"
                      + " before you can proceed.",
                      null, null);

        Login() {
            super("Make A Bid");
            id.setLabel("Enter Auction ID:");
            append(id);
            pd.setLabel("Enter PIN:");
            append(pd);
            append(curBid);
            append(incBid);
            append(minBid);
            append(bidText);
            addCommand(BACK_CMD);
            addCommand(NEXT_CMD);
            setCommandListener(this);

            ItemCommandListener icl = new MyItemCommandListener();
            confirm.append("dummy");
            confirm.addCommand(CANCEL_CMD);
            confirm.setCommandListener(this);

            auctionHyperlink.setDefaultCommand(AUCTION_CMD);
            auctionHyperlink.setItemCommandListener(icl); 

            submitButton.setDefaultCommand(SUBMIT_CMD);
            submitButton.setItemCommandListener(icl); 

            deleteFormItem(notice);
            notice.append(new StringItem("Your bid has been received:", ""));
            notice.append(new StringItem("Confirm #:", "12-B455-31"));
            notice.addCommand(BAND_CMD);
            notice.setCommandListener(this);
            loginAlert.setTimeout(Alert.FOREVER);

            auctionForm.append("dummy");
            auctionForm.addCommand(BACK_CMD);
            auctionForm.setCommandListener(this);
        }

        String bandName, bid, ticketID, _bandName;
        int _bandIndex;

        void setBid(String bandName, String bid) {
            this.bandName = bandName;
            this.bid = bid;
        }

        void setID(String bandName, String ticketID) {
            this.bandName = bandName;
            this.ticketID = ticketID;
        }

        void setBandAttributes(String bandName, int bandIndex)
        {
          _bandName = bandName;
          _bandIndex = bandIndex;
        }
        void doSubmit() {
            TimerService = new Timer();
            TimerClient timerClient = new TimerClient();
            TimerService.schedule(timerClient, 0, 1000);        
            display.setCurrent(submitMsg);
        }

        class MyItemCommandListener implements ItemCommandListener
        {
            public void commandAction(Command c,
                                      Item item) {

                if (c == SUBMIT_CMD) {
                    doSubmit();
                } else if (c == AUCTION_CMD) {
                    deleteFormItem(auctionForm);
                    auctionForm.setTitle("Auction:" +login.ticketID);
                    band.getAuctionData(auctionForm, login.bandName, 
					login._bandIndex);
                    display.setCurrent(auctionForm);
                }
            }
        }

        public void commandAction(Command c, Displayable s) {
            if (s instanceof Form) {
                Form obj = (Form) s;
                if (obj == this) {
                    if (c == BACK_CMD) {
                        display.setCurrent(band_ticket_Form);
                    } else if (c == NEXT_CMD) {
                      if ((id.getString().length() == 0) || 
                          (pd.getString().length() == 0)) { // ||
                          // (bidText.getString().length() == 0)) {
                        display.setCurrent(loginAlert);
                      } else {
                        login.setID(_bandName, band.getTicketID(_bandName,
                                           _bandIndex));

                        String bidAmt = bidText.getString();
                        System.err.println("\n\n #### bidAmt : " + bidAmt);

                        // check if bid text is valid, if not pop up an alert
                        if (bidAmt.equals("")) {
                            Alert errorAlert = new Alert("Alert", 
							 "The bid amount you have entered is invalid.",
                                          null, AlertType.ERROR);
                            errorAlert.setTimeout(Alert.FOREVER);
                            display.setCurrent(errorAlert);
                        } else {

                            deleteFormItem(login.confirm);
                            login.confirm.append(new StringItem("Please confirm this information is correct before submitting your bid", ""));
                            auctionHyperlink.setText(login.ticketID);
                            login.confirm.append(auctionHyperlink);
                            login.confirm.append(new StringItem("Band: " + 
						       login.bandName, ""));
                            login.confirm.append(new StringItem("Bid Amount:",
                                                                "$"+bidAmt));
                            login.confirm.append(submitButton);
 
                            display.setCurrent(login.confirm);
                       }
                      }

                    }
                } else if (obj == confirm) {
                    if (c == CANCEL_CMD) {
                        display.setCurrent(this);
                    }
                } else if (obj == notice) {
                    if (c == BAND_CMD) {
                      display.setCurrent(band_ticket_Form);
                    }
                } else if (obj == auctionForm) {
                    if (c == BACK_CMD) {
                      display.setCurrent(confirm);
                    }
                }
            }
        }
        private class TimerClient extends TimerTask {
            public final void run() {
                 if (submitGauge.getValue() == submitGauge.getMaxValue()) {
                        TimerService.cancel();
                        submitGauge.setValue(0);
                        display.setCurrent(notice);        
                } else {
                        submitGauge.setValue(submitGauge.getValue()+10);
                }        
            }
        }

    }



    class Band {
        private BandListTable table;

        class TicketItem {
            // Ticket info
            String name;   // item name i.e. band name
            String seat;   // seat number
            String sect;   // seat section
            String date;   // concert date
            String place;  // concert place
            String state;  // place's state

            // Bid info
            String id;        // id i.e. item number
            String numItem;   // quantity
            String begBid;    // beginning bid price
            String curBid;    // current bid price
            String incBid;    // bid increment
            String numBids;   // # of bids
            String endsAt;    // Bid close time

            TicketItem(String name,
                       String seat,
                       String sect,
                       String date,
                       String place,
                       String state,
                       String id,
                       String numItem,
                       String begBid,
                       String curBid,
                       String incBid,
                       String numBids,
                       String endsAt) {
                this.name = name;
                this.seat = seat;
                this.sect = sect;
                this.date = date;
                this.place = place;
                this.state = state;
                this.id = id;
                this.numItem = numItem;
                this.begBid = begBid;
                this.curBid = curBid;
                this.incBid = incBid;
                this.numBids = numBids;
                this.endsAt = endsAt;
            }

            TicketItem(String name) {
                this(name, "n/a", "n/a", "n/a", "n/a", "n/a", 
                     "0", "0", "0", "0", "0", "0", "0");
            }
        }

        class BandListTable {
            private Vector vec = new Vector();
            BandListTable() {
                vec = new Vector();
            }
            void put(int nth, Object obj) {
                Object o = null;
                try { 
                    o = vec.elementAt(nth);
                } catch (ArrayIndexOutOfBoundsException e) {
                    o = null;
                }
                if (o == null) {
                    vec.addElement(obj);
                } else {
                    vec.setElementAt(obj, nth);
                }
            }
            Object get(String bandName) {
                int num = vec.size();
                for (int i = 0; i < num; i++) {
                    Object obj = vec.elementAt(i);
                    if (obj instanceof Vector) {
                        Vector v = (Vector) obj;
                        TicketItem item = (TicketItem) v.elementAt(0);
                        if (item.name.equals(bandName)) {
                            return obj;
                        }
                    }
                }
                return null;
            }
            Object elementAt(int index) {
                return vec.elementAt(index);
            }
            void remove(int i) {
                Object o = null;
                try { 
                    o = vec.elementAt(i);
                } catch (ArrayIndexOutOfBoundsException e) {
                    o = null;
                }
                if (o != null) {
                    vec.removeElementAt(i);
                }
            }
            int size() {
                return vec.size();
            }
        }

        Band() {
            table = new BandListTable();
        

            TicketItem t;

            /**
             * 0: Alanis Morrisette
             */
            t = new TicketItem("Alanis Morrisette",
                               "18 & 19",
                               "XF, Row 17",
                               "Aug 23, 2002",
                               "PacBell Stadium",
                               "San Francisco, CA",
                               "#7720",
                               "2",
                               "45.00",
                               "69.00",
                               "2.00",
                               "3",
                               "12:00 am on Aug 16, 2002");
            add(0, "Alanis Morrisette", t);

            /**
             * 1: Envy
             */
            t = new TicketItem("Envy",
                               "10, 11, 12, 13",
                               "F9, Row A",
                               "Sep 7, 2002",
                               "Civic Center",
                               "Santa Cruz, CA",
                               "4509",
                               "4",
                               "100.00",
                               "101.00",
                               "1.00",
                               "1",
                               "9:45 pm on Sep 1, 2002");
            add(1, "Envy", t);

            t = new TicketItem("Envy",
                               "69 & 70",
                               "PIT Row 1",
                               "Aug 9, 2002",
                               "Monterey Civic Ampitheatre",
                               "Monterey, CA",
                               "3058",
                               "2",
                               "9.00",
                               "175.00",
                               "2.50",
                               "40",
                               "9:00 pm on Aug 2, 2002");
            add(1, "Envy", t);

            t = new TicketItem("Envy",
                               "5 & 6 & 7",
                               "PIT Rw 11",
                               "Aug 10, 2002",
                               "Monterey Civic Ampitheatre",
                               "Monterey, CA",
                               "3541",
                               "3",
                               "28.00",
                               "97.00",
                               "1.00",
                               "11",
                               "11:15 pm on Aug 2, 2002");
            add(1, "Envy", t);
            
            /**
	    * 2: JawBreakers
	    */
            t = new TicketItem("JawBreakers",
                               "General Admission",
                               "- NA -",
                               "Aug 3, 2002",
                               "Bottom of the Hill",
                               "San Francisco, CA",
                               "3489",
                               "2",
                               "30.00",
                               "44.00",
                               "2.00",
                               "5",
                               "11:30 pm on Jul 7, 2002");
            add(2, "JawBreakers", t);

            /**
             * 3: Jets to Brazil
             */
            t = new TicketItem("Jets to Brazil",
                               "General Admission",
                               "- NA -",
                               "Jul 19, 2001",
                               "The Fillmore",
                               "San Francisco, CA",
                               "3861",
                               "2",
                               "55.00",
                               "62.50",
                               "2.50",
                               "3",
                               "6:30 pm on Jul 12, 2002");
            add(3, "Jets to Brazil", t);

            t = new TicketItem("Jets to Brazil",
                               "4 & 6",
                               "N2 Row 5",
                               "Jul 18, 2002",
                               "Mountain View Center for the Performing Arts",
                               "Mountain View, CA",
                               "9916",
                               "2",
                               "65.00",
                               "200.00",
                               "10.00",
                               "12",
                               "9:30 pm on Jul 10, 2002");
            add(3, "Jets to Brazil", t);


            /**
             * 4: moby
             */
            t = new TicketItem("moby",
                               "General Admission",
                               "- NA -",
                               "Aug 3, 2002",
                               "Bottom of the Hill",
                               "San Francisco",
                               "1313",
                               "2",
                               "30.00",
                               "37.00",
                               "1.00",
                               "7",
                               "5:35 pm on Jul 27, 2002");
            add(4, "moby", t);

            /**
             * 5: Nerf Herders
             */
            t = new TicketItem("Nerf Herders",
                               "N & O",
                               "S, Row 14",
                               "Aug 3, 2002",
                               "3COM Park",
                               "San Francisco, CA",
                               "8120",
                               "2",
                               "80.00",
                               "125.00",
                               "2.00",
                               "5",
                               "11:30 am on Jul 7, 2002");
            add(5, "Nerf Herders", t);
            
	    /**
	    * 6: The Mr. T Experience
	    */
            t = new TicketItem("The Mr. T Experience",
                               "15 & 16",
                               "B, Row 14",
                               "Aug 30, 2002",
                               "3COM Park",
                               "San Francisco, CA",
                               "1770",
                               "2",
                               "100.00",
                               "175.00",
                               "2.00",
                               "5",
                               "11:30 pm on Aug 23, 2002");
            add(6, "The Mr. T Experience", t);

            /**
             * 7: Thirty Odd Foot of Grunts
             */
            t = new TicketItem("Thirty Odd Foot of Grunts",
                               "11 & 12",
                               "F Row J",
                               "Jul 19, 2001",
                               "Stugg's Ampitheatre",
                               "Austin, TX",
                               "3766",
                               "2",
                               "60.00",
                               "90.00",
                               "5.00",
                               "3",
                               "6:30 pm on Jul 12, 2002");
            add(7, "Thirty Odd Foot of Grunts", t);

            t = new TicketItem("Thirty Odd Foot of Grunts",
                               "4 & 6",
                               "N2 Row 5",
                               "Jul 18, 2002",
                               "Seattle Thunderdome",
                               "Seattle, WA",
                               "9135",
                               "2",
                               "70.00",
                               "200.00",
                               "10.00",
                               "12",
                               "9:30 pm on Jul 10, 2002");
            add(7, "Thirty Odd Foot of Grunts", t);

            t = new TicketItem("Thirty Odd Foot of Grunts",
                               "General Admission",
                               "- NA -",
                               "Jul 20, 2002",
                               "The Fillmore",
                               "San Francisco, CA",
                               "2722",
                               "2",
                               "75.00",
                               "110.00",
                               "2.00",
                               "12",
                               "4:00 pm on Jul 13, 2002");
            add(7, "Thirty Odd Foot of Grunts", t);
        }

        void add(String bandName) {
            add(table.size(), bandName, new TicketItem(bandName));
        }

        void add(int i, String bandName) {
            add(i, bandName, new TicketItem(bandName));
        }

        void add(int i, String bandName, TicketItem item) {
            Object obj = table.get(bandName);
            Vector vec = (obj == null) ? new Vector() : (Vector) obj;
            vec.addElement(item);
            table.put(i, vec);
        }

        void remove(int i) {
            table.remove(i);
        }

        String[] getList() {
            int num = table.size();
            String seq[] = new String[num];
            for (int i = 0; i < num; i++) {
                Vector vec = (Vector) table.elementAt(i);
                TicketItem item = (TicketItem) vec.elementAt(0);
                seq[i] = item.name;
            }
            return seq;
        }

        String getName(int nth) {
            Vector vec = (Vector) table.elementAt(nth);
            if (vec != null) {
                TicketItem item = (TicketItem) vec.elementAt(0);
                if (item != null) {
                    return item.name;
                }
            }
            return null;
        }

        String[] getTicketList(String bandName) {
            Object obj = table.get(bandName);
            if (obj == null) {
                return null;
            }

            Vector vec = (Vector) obj;
            int num = vec.size();
            String seq[] = new String[num];

            for (int i = 0; i < num; i++) {
                TicketItem item = (TicketItem) vec.elementAt(i);
                seq[i] = new String(
                    "#" + item.id + " $" + item.curBid + "\n" +
                    item.place + ", " + item.state + "\n" +
                    item.sect + "," + item.seat + " ...");
            }

            return seq;
        }

        TicketItem getTicketItem(String bandName, int nth) {
            Object obj = table.get(bandName);
            if (obj == null) {
                return null;
            }
            Vector vec = (Vector) obj;
            return (TicketItem) vec.elementAt(nth);
        }

        String getTicketID(String bandName, int nth) {
            TicketItem item = getTicketItem(bandName, nth);
            if (item == null) {
                return null;
            }
            return item.id;
        }

        String getTicketDataTitle(String bandName, int nth) {
            TicketItem item = getTicketItem(bandName, nth);
            if (item == null) {
                return null;
            }
            return item.name + " " + item.id + ":";
        }

        void getTicketData(Form form, String bandName, int nth) {
            TicketItem item = getTicketItem(bandName, nth);
            if (item == null) {
                return;
            }
            form.append(new StringItem("Quantity:",  item.numItem + " tickets"));
            form.append(new StringItem("Bid starts at:", "$"+item.begBid));
            form.append(new StringItem("Current bid:",   "$"+item.curBid));
            form.append(new StringItem("Number of Bids:",    item.numBids));
            form.append(new StringItem("Bidding ends at:", item.endsAt));
            // form.append(new StringItem("Sec:",     item.sect));
            form.append(new StringItem("Seat(s):", item.seat));
            form.append(new StringItem("Concert Date:",    item.date));
            form.append(new StringItem("Concert Venue:",   
                                 item.place+","+item.state));
        }

        void getAuctionData(Form form, String bandName, int nth) {
            TicketItem item = getTicketItem(bandName, nth);
            if (item == null) {
                return;
            }
            form.append(new StringItem("Band Name:", item.name + " tickets"));
            form.append(new StringItem("Concert Date:",    item.date));
            form.append(new StringItem("Quantity:",  item.numItem));
            form.append(new StringItem("Concert Venue:",   
                                 item.place+","+item.state));
        }

        String getCurrentBid(String bandName, int nth) {
            TicketItem item = getTicketItem(bandName, nth);
            if (item == null) {
                return "";
            }
            return item.curBid;
        }

        boolean isTicketData(String bandName, String itemID) {
            return (getTicketItem(bandName, itemID) == null) ? false : true;
        }
        
        TicketItem getTicketItem(String bandName, String itemID) {
            Object obj = table.get(bandName);
            if (obj == null) {
                return null;
            }

            Vector vec = (Vector) obj;
            int num = vec.size();

            for (int i = 0; i < num; i++) {
                TicketItem item = (TicketItem) vec.elementAt(i);
                if (item.id.equals(itemID)) {
                    return item;
                }
            }

            return null;
        }

        void getTicketBidTitle(Form f, String bandName, int itemID) {
            TicketItem item = getTicketItem(bandName, itemID);
            long min = stringToLong1000(item.curBid) + 
                stringToLong1000(item.incBid);
            String minBid = long1000ToString(min);
            
            login.curBid.setText("$"+item.curBid);
            login.incBid.setText("$"+item.incBid);
            login.minBid.setText("$"+minBid);
        }

        String def_ticker = "Ricky Martin 2 tix $90.00 2 tix $110.00"
            + " 2 tix $200.00 Tina Turner 2 tix $65.00";
        String alt_ticker = "Alanis Morisette 2 tix $58.00 4 tix $115.00"
            + " Fiona Apple 2 tix $37.00";

        String toTickerString(boolean[] selected) {
            String s = "";

            if (selected == null) {
                return alt_ticker;
            }

            // Rock Bands
            if (selected[0]) {
                s += def_ticker;
            }
            // Pop
            if (selected[1]) {
            }
            // Country
            if (selected[2]) {
            }
            // Alternative
            if (selected[3]) {
                if (s.length() > 0) {
                    s += " ";
                }
                s += alt_ticker;
            }
            // Jazz
            if (selected[4]) {
            }
            // Classical
            if (selected[5]) {
            }

            return s;
        }

        long stringToLong1000(String s) {
            if (s == null || s.equals("")) {
                return 0;
            }
            try {
                int index = s.indexOf('.');
                if (index == -1)
                    return (long)1000*Integer.parseInt(s);

                long integerPart = 
		    1000*Integer.parseInt(s.substring(0, index));
                String fracString = s.substring(index+1);
                int multBy = 0;
                switch (fracString.length()) {
                case 0:
                    multBy = 0;
                    break;
                case 1:
                    multBy = 100;
                    break;
                case 2:
                    multBy = 10;
                    break;
                case 3:
                    multBy = 1;
                    break;
                }

                long fractionalPart = multBy * Integer.parseInt(fracString);
                return integerPart + fractionalPart;
            } catch (NumberFormatException nfe) {
                return 0;
            }
        }

        String long1000ToString(long l) {
            if (l == 0)
                return "0";
            String s = String.valueOf(l);
            if (s.length() < 4)
                return "0";
            String newStr = s.substring(0, s.length()-3) +"."+ 
                s.substring(s.length()-2);
            return newStr;
        }
    } // end  band

    class RmBand extends List implements CommandListener {
        RmBand() {
            super("Remove Bands", Choice.MULTIPLE);
            String[] list = band.getList();
            for (int i = 0; i < list.length; i++) {
                append(list[i], null);
            }
               addCommand(BACK_CMD);
            addCommand(SAVE_CMD);
            setCommandListener(this);
        }
        public void commandAction(Command c, Displayable s) {

            if (c == SAVE_CMD) {                
                  // Make the ChoiceGroup invisible before doing
                // reconstruction
                boolean[] ret = new boolean[size()];
                getSelectedFlags(ret);
                removeBandList(ret);

                // go back to band_ticket_Form after save
                display.setCurrent(savedMsg, band_ticket_Form);
              } else if (c == BACK_CMD) {
                display.setCurrent(band_ticket_Form);
            }

        }
        void reset() {
            reconstructList(this, band.getList());
        }
        void removeBandList(boolean[] t) {
            int i = t.length;
            while (--i >= 0) {
                if (t[i]) {
                    band.remove(i);
                }
            }

            reconstructBandTicketForm(band.getList());
            reconstructList(this, band.getList());
        }
    } // end rm band

    class UpdateAlert {
        String band = "", bid = "";
         Alert soundAlert;
        UpdateAlert() {
            soundAlert = new Alert("Alert", "",
                             null, AlertType.ALARM);
            soundAlert.setTimeout(Alert.FOREVER);
        }
        void set(String band, String bid) {
            this.band = band;
            this.bid = bid;
        }
        boolean hasDataToUpdate() {
            return ((band != null) && (band != "") &&
                    (bid != null) && (bid != ""));
        }
        void show() {
            // no-op if band & bid aren't set
            if (hasDataToUpdate()) {
                String s = new String(band + "\n" + 
                                  "ticket bids\n" +
                                  "have reached\n" +
                                  "$" + bid);
                soundAlert.setString(s);
                display.setCurrent(soundAlert);
            }
        }

    }// end UpdateAlert 


    void updateBandList(String list) {
        if (list.length() == 0) {
            return;
        }

        Vector vec = new Vector();
        int fidx = 0;
        while (true) {
            int idx = list.indexOf(',', fidx);
            if (idx == -1) {
                vec.addElement(list.substring(fidx));
                break;
            }
            vec.addElement(list.substring(fidx, idx));
            fidx = idx + 1;
        }

        for (int i = 0; i < vec.size(); i++) {
            String str = (String)vec.elementAt(i);
            int j = 0;
            int len = str.length();
            for (; j < len; j++) {
                if (str.charAt(j) != ' ') {
                    break;
                }
            }
            if (j == len) {
                continue;
            }
            if (j == 0) {
                band.add(str);
            } else {
                band.add(str.substring(j));
            }
        }
        reconstructBandTicketForm(band.getList());
        rmBand.reset();
    }

    void reconstructBandTicketForm(String[] items) {
        if (items == null || items.length == 0) {
            bandCg.deleteAll();
            auctionCg.setLabel(null);
            auctionCg.deleteAll();
            return;
        }

        reconstructList(bandCg, items);

        bandCg.setSelectedIndex(0, true);
        String bandName = band.getName(bandCg.getSelectedIndex());
        auctionCg.setLabel(bandName + " Auctions:");
        reconstructList(auctionCg, band.getTicketList(bandName));
    }

    void reconstructList(Choice list, String[] items) {
        list.deleteAll();
        for (int i = 0; i < items.length; i++) {
            list.append(items[i], null);
        }
    }

    class SetMenuForm extends Form 
        implements CommandListener, ItemStateListener {

        Timer timerService = new Timer();
        ChoiceGroup tickerCg, updatesCg;
        Gauge gauge;
        DateField dateTimeItem;

        int updateChoice, volumeValue;
        boolean musicChoice[];
        boolean systemCurrentDate = true;
        long setTimeMillisDelta, curTimeMillisDelta;

        SetMenuForm() {
            super("Settings");

            tickerCg = new ChoiceGroup("Ticker Display", Choice.MULTIPLE);
            tickerCg.append("Rock", null);
            tickerCg.append("Pop", null);
            tickerCg.append("Country", null);
            tickerCg.append("Alternative", null);
            tickerCg.append("Jazz", null);
            tickerCg.append("Classical", null);
            musicChoice = new boolean[] {false, false, false, 
                                         true, false, false};
            append(tickerCg);

            updatesCg = new ChoiceGroup("Updates", Choice.EXCLUSIVE);
            updatesCg.append("Continuous", null);
            updatesCg.append("15 minutes", null);
            updatesCg.append("30 minutes", null);
            updatesCg.append("1 hour", null);
            updatesCg.append("3 hours", null);
            updateChoice = 0;
            append(updatesCg);

            /* Set Number */
            gauge = new Gauge(null, true, 40, 0);
            volumeValue = 0;
            append(gauge);
            setItemStateListener(this);

            setTimeMillisDelta = curTimeMillisDelta = 0;

            dateTimeItem = new DateField("Set Date:", DateField.DATE_TIME) {
                public void showNotify() {
                    long millis = System.currentTimeMillis();
                    if (curTimeMillisDelta != 0) {
                        millis -= curTimeMillisDelta;
                    }
                    setDate(new java.util.Date(millis));
                }
            };

            append(dateTimeItem);

            settings(musicChoice, updateChoice, 
                     volumeValue, setTimeMillisDelta);

            addCommand(BACK_CMD);
            addCommand(SAVE_CMD);
            setCommandListener(this);
        }

        public void settings(boolean musicChoice[], int updateChoice,
                             int volumeValue, long curTimeMillisDelta) {
            tickerCg.setSelectedFlags(musicChoice);
            updatesCg.setSelectedIndex(updateChoice, true);
            gauge.setValue(volumeValue);
            gauge.setLabel("Set Alert Volume: " + volumeValue);

            long millis = System.currentTimeMillis();
            if (curTimeMillisDelta != 0) { 
                millis -= curTimeMillisDelta;
            }
            dateTimeItem.setDate(new java.util.Date(millis));
        }

        public void commandAction(Command c, Displayable s) {
            if (c == SAVE_CMD) {
                tickerCg.getSelectedFlags(musicChoice);

                String str = band.toTickerString(musicChoice);
                if (str != null) {
                    ticker.setString(str);
                }

                updateChoice = updatesCg.getSelectedIndex();
               
                volumeValue = gauge.getValue();

                setTimeMillisDelta = curTimeMillisDelta;

                if (updateAlert.hasDataToUpdate()) {
                    int idx = updatesCg.getSelectedIndex();
                    TimerTask timerClient = new TimerTask() {
                        public final void run() {
                          updateAlert.show();
                        }
                    };
                    switch (idx) {
                    case 0:
                        timerService.schedule(timerClient, 3000);
                        break;
                    case 1:
                        timerService.schedule(timerClient, 3000);
                        break;
                    case 2:
                        timerService.schedule(timerClient, 3000);
                        break;
                    case 3:
                        timerService.schedule(timerClient, 3000);
                        break;
                    case 4:
                        timerService.schedule(timerClient, 3000);
                        break;
                    }
                }
                display.setCurrent(savedMsg, band_ticket_Form);

            } else if (c == BACK_CMD) {
                display.setCurrent(band_ticket_Form);
                settings(musicChoice, updateChoice, 
                         volumeValue, setTimeMillisDelta);
            }
        }

        public void itemStateChanged(Item item) { 
            if (item == gauge) {
                int currentValue = gauge.getValue();
                gauge.setLabel("Set Alert Volume: " + 
                                String.valueOf(currentValue));
            } else if (item == dateTimeItem) {
                curTimeMillisDelta = System.currentTimeMillis() - 
                    dateTimeItem.getDate().getTime();
            }
        }
    }

    void deleteFormItem(Form f) {
        int num = f.size();
        while (--num >= 0) {
            f.delete(num);
        }
    }
}
