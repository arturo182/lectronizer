#include "utils.h"

#include <QDebug>
#include <QMap>

static const QMap<QString, QString> phoneCodes =
{
    { "Afghanistan", "93" },
    { "Åland Islands", "358" },
    { "Albania", "355" },
    { "Algeria", "213" },
    { "American Samoa", "1684" },
    { "Andorra", "376" },
    { "Angola", "244" },
    { "Anguilla", "1264" },
    { "Antarctica", "" },
    { "Antigua and Barbuda", "1268" },
    { "Argentina", "54" },
    { "Armenia", "374" },
    { "Aruba", "297" },
    { "Australia", "61" },
    { "Austria", "43" },
    { "Azerbaijan", "994" },
    { "Bahamas", "1242" },
    { "Bahrain", "973" },
    { "Bangladesh", "880" },
    { "Barbados", "1246" },
    { "Belarus", "375" },
    { "Belgium", "32" },
    { "Belize", "501" },
    { "Benin", "229" },
    { "Bermuda", "1441" },
    { "Bhutan", "975" },
    { "Bolivia", "591" },
    { "Bosnia", "387" },
    { "Botswana", "267" },
    { "Bouvet Island", "55" },
    { "Brazil", "55" },
    { "British Indian Ocean Territory", "246" },
    { "British Virgin Islands", "1284" },
    { "Brunei", "673" },
    { "Bulgaria", "359" },
    { "Burkina Faso", "226" },
    { "Burundi", "257" },
    { "Cambodia", "855" },
    { "Cameroon", "237" },
    { "Canada", "1" },
    { "Cape Verde", "238" },
    { "Caribbean Netherlands", "599" },
    { "Cayman Islands", "1345" },
    { "Central African Republic", "236" },
    { "Chad", "235" },
    { "Chile", "56" },
    { "China", "86" },
    { "Christmas Island", "61" },
    { "Cocos (Keeling) Islands", "61" },
    { "Colombia", "57" },
    { "Comoros", "269" },
    { "Congo - Brazzaville", "242" },
    { "Congo - Kinshasa", "243" },
    { "Cook Islands", "682" },
    { "Costa Rica", "506" },
    { "Côte d’Ivoire", "225" },
    { "Croatia", "385" },
    { "Cuba", "53" },
    { "Curaçao", "599" },
    { "Cyprus", "357" },
    { "Czechia", "420" },
    { "Denmark", "45" },
    { "Djibouti", "253" },
    { "Dominica", "1767" },
    { "Dominican Republic", "1809" },
    { "Ecuador", "593" },
    { "Egypt", "20" },
    { "El Salvador", "503" },
    { "Equatorial Guinea", "240" },
    { "Eritrea", "291" },
    { "Estonia", "372" },
    { "Eswatini", "268" },
    { "Ethiopia", "251" },
    { "Falkland Islands", "500" },
    { "Faroe Islands", "298" },
    { "Fiji", "679" },
    { "Finland", "358" },
    { "France", "33" },
    { "French Guiana", "594" },
    { "French Polynesia", "689" },
    { "French Southern Territories", "262" },
    { "Gabon", "241" },
    { "Gambia", "220" },
    { "Georgia", "995" },
    { "Germany", "49" },
    { "Ghana", "233" },
    { "Gibraltar", "350" },
    { "Greece", "30" },
    { "Greenland", "299" },
    { "Grenada", "1473" },
    { "Guadeloupe", "590" },
    { "Guam", "1671" },
    { "Guatemala", "502" },
    { "Guernsey", "441481" },
    { "Guinea", "224" },
    { "Guinea-Bissau", "245" },
    { "Guyana", "592" },
    { "Haiti", "509" },
    { "Heard & McDonald Islands", "672" },
    { "Honduras", "504" },
    { "Hong Kong", "852" },
    { "Hungary", "36" },
    { "Iceland", "354" },
    { "India", "91" },
    { "Indonesia", "62" },
    { "Iran", "98" },
    { "Iraq", "964" },
    { "Ireland", "353" },
    { "Isle of Man", "441624" },
    { "Israel", "972" },
    { "Italy", "39" },
    { "Jamaica", "1876" },
    { "Japan", "81" },
    { "Jersey", "441534" },
    { "Jordan", "962" },
    { "Kazakhstan", "7" },
    { "Kenya", "254" },
    { "Kiribati", "686" },
    { "Kuwait", "965" },
    { "Kyrgyzstan", "996" },
    { "Laos", "856" },
    { "Latvia", "371" },
    { "Lebanon", "961" },
    { "Lesotho", "266" },
    { "Liberia", "231" },
    { "Libya", "218" },
    { "Liechtenstein", "423" },
    { "Lithuania", "370" },
    { "Luxembourg", "352" },
    { "Macao", "853" },
    { "Madagascar", "261" },
    { "Malawi", "265" },
    { "Malaysia", "60" },
    { "Maldives", "960" },
    { "Mali", "223" },
    { "Malta", "356" },
    { "Marshall Islands", "692" },
    { "Martinique", "596" },
    { "Mauritania", "222" },
    { "Mauritius", "230" },
    { "Mayotte", "262" },
    { "Mexico", "52" },
    { "Micronesia", "691" },
    { "Moldova", "373" },
    { "Monaco", "377" },
    { "Mongolia", "976" },
    { "Montenegro", "382" },
    { "Montserrat", "1664" },
    { "Morocco", "212" },
    { "Mozambique", "258" },
    { "Myanmar", "95" },
    { "Namibia", "264" },
    { "Nauru", "674" },
    { "Nepal", "977" },
    { "Netherlands", "31" },
    { "New Caledonia", "687" },
    { "New Zealand", "64" },
    { "Nicaragua", "505" },
    { "Niger", "227" },
    { "Nigeria", "234" },
    { "Niue", "683" },
    { "Norfolk Island", "672" },
    { "Northern Mariana Islands", "1670" },
    { "North Korea", "850" },
    { "North Macedonia", "389" },
    { "Norway", "47" },
    { "Oman", "968" },
    { "Pakistan", "92" },
    { "Palau", "680" },
    { "Palestine", "970" },
    { "Panama", "507" },
    { "Papua New Guinea", "675" },
    { "Paraguay", "595" },
    { "Peru", "51" },
    { "Philippines", "63" },
    { "Pitcairn Islands", "870" },
    { "Poland", "48" },
    { "Portugal", "351" },
    { "Puerto Rico", "1787 and 1939" },
    { "Qatar", "974" },
    { "Réunion", "262" },
    { "Romania", "40" },
    { "Russia", "7" },
    { "Rwanda", "250" },
    { "Samoa", "685" },
    { "San Marino", "378" },
    { "São Tomé & Príncipe", "239" },
    { "Saudi Arabia", "966" },
    { "Senegal", "221" },
    { "Serbia", "381" },
    { "Seychelles", "248" },
    { "Sierra Leone", "232" },
    { "Singapore", "65" },
    { "Sint Maarten", "599" },
    { "Slovakia", "421" },
    { "Slovenia", "386" },
    { "Solomon Islands", "677" },
    { "Somalia", "252" },
    { "South Africa", "27" },
    { "South Georgia & South Sandwich Islands", "500" },
    { "South Korea", "82" },
    { "South Sudan", "211" },
    { "Spain", "34" },
    { "Sri Lanka", "94" },
    { "St. Barthélemy", "590" },
    { "St. Helena", "290" },
    { "St. Kitts & Nevis", "1869" },
    { "St. Lucia", "1758" },
    { "St. Martin", "590" },
    { "St. Pierre & Miquelon", "508" },
    { "St. Vincent & Grenadines", "1784" },
    { "Sudan", "249" },
    { "Suriname", "597" },
    { "Svalbard & Jan Mayen", "47" },
    { "Sweden", "46" },
    { "Switzerland", "41" },
    { "Syria", "963" },
    { "Taiwan", "886" },
    { "Tajikistan", "992" },
    { "Tanzania", "255" },
    { "Thailand", "66" },
    { "Timor-Leste", "670" },
    { "Togo", "228" },
    { "Tokelau", "690" },
    { "Tonga", "676" },
    { "Trinidad & Tobago", "1868" },
    { "Tunisia", "216" },
    { "Turkey", "90" },
    { "Turkmenistan", "993" },
    { "Turks & Caicos Islands", "1649" },
    { "Tuvalu", "688" },
    { "Uganda", "256" },
    { "UK", "44" },
    { "Ukraine", "380" },
    { "United Arab Emirates", "971" },
    { "United States", "1" },
    { "Uruguay", "598" },
    { "U.S. Outlying Islands", "1" },
    { "U.S. Virgin Islands", "1340" },
    { "Uzbekistan", "998" },
    { "Vanuatu", "678" },
    { "Vatican City", "379" },
    { "Venezuela", "58" },
    { "Vietnam", "84" },
    { "Wallis & Futuna", "681" },
    { "Western Sahara", "212" },
    { "Yemen", "967" },
    { "Zambia", "260" },
    { "Zimbabwe", "263" },
};

static const QMap<QString, QString> usStateCodes =
{
    { "alabama", "AL" },
    { "alaska", "AK" },
    { "arizona", "AZ" },
    { "arkansas", "AR" },
    { "california", "CA" },
    { "colorado", "CO" },
    { "connecticut", "CT" },
    { "delaware", "DE" },
    { "florida", "FL" },
    { "georgia", "GA" },
    { "hawaii", "HI" },
    { "idaho", "ID" },
    { "illinois", "IL" },
    { "indiana", "IN" },
    { "iowa", "IA" },
    { "kansas", "KS" },
    { "kentucky", "KY" },
    { "louisiana", "LA" },
    { "maine", "ME" },
    { "maryland", "MD" },
    { "massachusetts", "MA" },
    { "michigan", "MI" },
    { "minnesota", "MN" },
    { "mississippi", "MS" },
    { "missouri", "MO" },
    { "montana", "MT" },
    { "nebraska", "NE" },
    { "nevada", "NV" },
    { "new hampshire", "NH" },
    { "new jersey", "NJ" },
    { "new mexico", "NM" },
    { "new york", "NY" },
    { "north carolina", "NC" },
    { "north dakota", "ND" },
    { "ohio", "OH" },
    { "oklahoma", "OK" },
    { "oregon", "OR" },
    { "pennsylvania", "PA" },
    { "rhode island", "RI" },
    { "south carolina", "SC" },
    { "south dakota", "SD" },
    { "tennessee", "TN" },
    { "texas", "TX" },
    { "utah", "UT" },
    { "vermont", "VT" },
    { "virginia", "VA" },
    { "washington", "WA" },
    { "west virginia", "WV" },
    { "wisconsin", "WI" },
    { "wyoming", "WY" }
};

// Friendly:
//  - "Yesterday" for yesterday
//  - "Today" for today
//  - "Tomorrow" for tomorrow
//  - <day name> for this week
//  - "Last <day name>" for last week
//  - "Next <day name>" for next week
// and a full date for everything else
QString textDate(const QDateTime &date, const SharedData &shared)
{
    const QDateTime now = QDateTime::currentDateTime();
    const int days = date.daysTo(now);
    const int dowNow = now.date().dayOfWeek();
    const int dowDate = dowNow + -days; // 1 - Monday, 7 - Sunday
    const QString timeStr = date.time().toString("hh:mm");

    if (!shared.friendlyDate || (dowDate < -6) || (dowDate > 14))
        return date.toString(shared.dateFormat);

    if (days == -1)
        return QObject::tr("Tomorrow, ") + timeStr;

    if (days == 0)
        return QObject::tr("Today, ") + timeStr;

    if (days == 1)
        return QObject::tr("Yesterday, ") + timeStr;

    if ((dowDate >= -6) && (dowDate <= 0))
        return QObject::tr("Last ") + date.toString("dddd, hh:mm");

    if ((dowDate > 7) && (dowDate <= 14))
        return QObject::tr("Next ") + date.toString("dddd, hh:mm");

    return date.toString("dddd, hh:mm");
}

QString sanitizePhoneNumber(const QString &phone, const QString &country, const SharedData &shared)
{
    const bool hadPrefix = phone.startsWith('+') || phone.startsWith('0');

    QString result = phone;

    if (result.isEmpty())
        return result;

    result.remove('(');
    result.remove(')');

    result.remove('+');
    if (result.startsWith("00")) {
        result = result.mid(2);
    } else if (result.startsWith('0')) {
        result = result.mid(1);
    }

    if (shared.phoneRemoveDashes)
        result.remove('-');

    if (shared.phoneRemoveSpaces)
        result.remove(' ');

    if (shared.phoneAddCountryCode) {
        if (phoneCodes.contains(country)) {
            const QString code = phoneCodes[country];
            if (!result.startsWith(code))
                result.prepend(code);

            result.prepend(shared.phoneUsePlusPrefix ? "+" : "00");
        } else {
            qDebug() << "Country" << country << "is not known!";
        }
    } else if (hadPrefix) {
        result.prepend(shared.phoneUsePlusPrefix ? "+" : "00");
    }


    return result;
}

QString shortenUsState(const QString &country, const QString &state)
{
    if (country != "United States")
        return state;

    return usStateCodes.value(state.toLower(), state);
}
