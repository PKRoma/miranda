{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_talk
 * Description : Header for the talk2 plugin, if somebody wants
 *               to access the function from a different module
 * Author      : Christian Kästner
 * Date        : 29.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_talk;

interface

const
  //sends the text to a text2speech engine
  //wParam=pchar(text)
  MS_TALK_SPEAKTEXT='Talk/SpeakText';

const
  //stop any talk activity
  //no params
  MS_TALK_STOPSPEAK='Talk/StopSpeak';

implementation

end.
