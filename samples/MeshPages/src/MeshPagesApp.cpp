#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/SdfText.h"
#include "cinder/gl/SdfTextMesh.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const float kPageWidth = 512;
const float kPageBorder = 16;
const float kPageTextWidth = kPageWidth - ( 2 * kPageBorder );

std::vector<std::string> sStrings = {
	"I hope you will be ready to own publicly, whenever you shall be called to it, that by your great and frequent urgency you prevailed on me to publish a very loose and uncorrect account of my travels, with directions to hire some young gentleman of either university to put them in order, and correct the style, as my cousin Dampier did, by my advice, in his book called \"A Voyage round the world.\"  But I do not remember I gave you power to consent that any thing should be omitted, and much less that any thing should be inserted; therefore, as to the latter, I do here renounce every thing of that kind; particularly a paragraph about her majesty Queen Anne, of most pious and glorious memory; although I did reverence and esteem her more than any of human species. But you, or your interpolator, ought to have considered, that it was not my inclination, so was it not decent to praise any animal of our composition before my master Houyhnhnm: And besides, the fact was altogether false; for to my knowledge, being in England during some part of her majesty's reign, she did govern by a chief minister; nay even by two successively, the first whereof was the lord of Godolphin, and the second the lord of Oxford; so that you have made me say the thing that was not. Likewise in the account of the academy of projectors, and several passages of my discourse to my master Houyhnhnm, you have either omitted some material circumstances, or minced or changed them in such a manner, that I do hardly know my own work. When I formerly hinted to you something of this in a letter, you were pleased to answer that you were afraid of giving offence; that people in power were very watchful over the press, and apt not only to interpret, but to punish every thing which looked like an innuendo (as I think you call it. But, pray how could that which I spoke so many years ago, and at about five thousand leagues distance, in another reign, be applied to any of the Yahoos, who now are said to govern the herd; especially at a time when I little thought, or feared, the unhappiness of living under them?  Have not I the most reason to complain, when I see these very Yahoos carried by Houyhnhnms in a vehicle, as if they were brutes, and those the rational creatures?  And indeed to avoid so monstrous and detestable a sight was one principal motive of my retirement hither.",
	"Thus much I thought proper to tell you in relation to yourself, and to the trust I reposed in you. I do, in the next place, complain of my own great want of judgment, in being prevailed upon by the entreaties and false reasoning of you and some others, very much against my own opinion, to suffer my travels to be published. Pray bring to your mind how often I desired you to consider, when you insisted on the motive of public good, that the Yahoos were a species of animals utterly incapable of amendment by precept or example: and so it has proved; for, instead of seeing a full stop put to all abuses and corruptions, at least in this little island, as I had reason to expect; behold, after above six months warning, I cannot learn that my book has produced one single effect according to my intentions. I desired you would let me know, by a letter, when party and faction were extinguished; judges learned and upright; pleaders honest and modest, with some tincture of common sense, and Smithfield blazing with pyramids of law books; the young nobility's education entirely changed; the physicians banished; the female Yahoos abounding in virtue, honour, truth, and good sense; courts and levees of great ministers thoroughly weeded and swept; wit, merit, and learning rewarded; all disgracers of the press in prose and verse condemned to eat nothing but their own cotton, and quench their thirst with their own ink. These, and a thousand other reformations, I firmly counted upon by your encouragement; as indeed they were plainly deducible from the precepts delivered in my book. And it must be owned, that seven months were a sufficient time to correct every vice and folly to which Yahoos are subject, if their natures had been capable of the least disposition to virtue or wisdom. Yet, so far have you been from answering my expectation in any of your letters; that on the contrary you are loading our carrier every week with libels, and keys, and reflections, and memoirs, and second parts; wherein I see myself accused of reflecting upon great state folk; of degrading human nature (for so they have still the confidence to style it, and of abusing the female sex. I find likewise that the writers of those bundles are not agreed among themselves; for some of them will not allow me to be the author of my own travels; and others make me author of books to which I am wholly a stranger.",
	"I find likewise that your printer has been so careless as to confound the times, and mistake the dates, of my several voyages and returns; neither assigning the true year, nor the true month, nor day of the month: and I hear the original manuscript is all destroyed since the publication of my book; neither have I any copy left: however, I have sent you some corrections, which you may insert, if ever there should be a second edition: and yet I cannot stand to them; but shall leave that matter to my judicious and candid readers to adjust it as they please.I hear some of our sea Yahoos find fault with my sea-language, as not proper in many parts, nor now in use. I cannot help it. In my first voyages, while I was young, I was instructed by the oldest mariners, and learned to speak as they did. But I have since found that the sea Yahoos are apt, like the land ones, to become new-fangled in their words, which the latter change every year; insomuch, as I remember upon each return to my own country their old dialect was so altered, that I could hardly understand the new. And I observe, when any Yahoo comes from London out of curiosity to visit me at my house, we neither of us are able to deliver our conceptions in a manner intelligible to the other. If the censure of the Yahoos could any way affect me, I should have great reason to complain, that some of them are so bold as to think my book of travels a mere fiction out of mine own brain, and have gone so far as to drop hints, that the Houyhnhnms and Yahoos have no more existence than the inhabitants of Utopia.",
	"Indeed I must confess, that as to the people of Lilliput, Brobdingrag (for so the word should have been spelt, and not erroneously Brobdingnag, and Laputa, I have never yet heard of any Yahoo so presumptuous as to dispute their being, or the facts I have related concerning them; because the truth immediately strikes every reader with conviction. And is there less probability in my account of the Houyhnhnms or Yahoos, when it is manifest as to the latter, there are so many thousands even in this country, who only differ from their brother brutes in Houyhnhnmland, because they use a sort of jabber, and do not go naked?  I wrote for their amendment, and not their approbation. The united praise of the whole race would be of less consequence to me, than the neighing of those two degenerate Houyhnhnms I keep in my stable; because from these, degenerate as they are, I still improve in some virtues without any mixture of vice. Do these miserable animals presume to think, that I am so degenerated as to defend my veracity?  Yahoo as I am, it is well known through all Houyhnhnmland, that, by the instructions and example of my illustrious master, I was able in the compass of two years (although I confess with the utmost difficulty to remove that infernal habit of lying, shuffling, deceiving, and equivocating, so deeply rooted in the very souls of all my species; especially the Europeans. I have other complaints to make upon this vexatious occasion; but I forbear troubling myself or you any further. I must freely confess, that since my last return, some corruptions of my Yahoo nature have revived in me by conversing with a few of your species, and particularly those of my own family, by an unavoidable necessity; else I should never have attempted so absurd a project as that of reforming the Yahoo race in this kingdom: But I have now done with all such visionary schemes for ever.",
	"The author gives some account of himself and family. His first inducements to travel. He is shipwrecked, and swims for his life. Gets safe on shore in the country of Lilliput; is made a prisoner, and carried up the country. My father had a small estate in Nottinghamshire: I was the third of five sons. He sent me to Emanuel College in Cambridge at fourteen years old, where I resided three years, and applied myself close to my studies; but the charge of maintaining me, although I had a very scanty allowance, being too great for a narrow fortune, I was bound apprentice to Mr. James Bates, an eminent surgeon in London, with whom I continued four years. My father now and then sending me small sums of money, I laid them out in learning navigation, and other parts of the mathematics, useful to those who intend to travel, as I always believed it would be, some time or other, my fortune to do. When I left Mr. Bates, I went down to my father: where, by the assistance of him and my uncle John, and some other relations, I got forty pounds, and a promise of thirty pounds a year to maintain me at Leyden: there I studied physic two years and seven months, knowing it would be useful in long voyages. Soon after my return from Leyden, I was recommended by my good master, Mr. Bates, to be surgeon to the Swallow, Captain Abraham Pannel, commander; with whom I continued three years and a half, making a voyage or two into the Levant, and some other parts. When I came back I resolved to settle in London; to which Mr. Bates, my master, encouraged me, and by him I was recommended to several patients. I took part of a small house in the Old Jewry; and being advised to alter my condition, I married Mrs. Mary Burton, second daughter to Mr. Edmund Burton, hosier, in Newgate-street, with whom I received four hundred pounds for a portion. But my good master Bates dying in two years after, and I having few friends, my business began to fail; for my conscience would not suffer me to imitate the bad practice of too many among my brethren. Having therefore consulted with my wife, and some of my acquaintance, I determined to go again to sea. I was surgeon successively in two ships, and made several voyages, for six years, to the East and West Indies, by which I got some addition to my fortune. My hours of leisure I spent in reading the best authors, ancient and modern, being always provided with a good number of books; and when I was ashore, in observing the manners and dispositions of the people, as well as learning their language; wherein I had a great facility, by the strength of my memory.The last of these voyages not proving very fortunate, I grew weary of the sea, and intended to stay at home with my wife and family. I removed from the Old Jewry to Fetter Lane, and from thence to Wapping, hoping to get business among the sailors; but it would not turn to account. After three years expectation that things would mend, I accepted an advantageous offer from Captain William Prichard, master of the Antelope, who was making a voyage to the South Sea. We set sail from Bristol, May 4, 1699, and our voyage was at first very prosperous.",
	"It would not be proper, for some reasons, to trouble the reader with the particulars of our adventures in those seas; let it suffice to inform him, that in our passage from thence to the East Indies, we were driven by a violent storm to the north-west of Van Diemen's Land. By an observation, we found ourselves in the latitude of 30 degrees 2 minutes south. Twelve of our crew were dead by immoderate labour and ill food; the rest were in a very weak condition. On the 5th of November, which was the beginning of summer in those parts, the weather being very hazy, the seamen spied a rock within half a cable's length of the ship; but the wind was so strong, that we were driven directly upon it, and immediately split. Six of the crew, of whom I was one, having let down the boat into the sea, made a shift to get clear of the ship and the rock. We rowed, by my computation, about three leagues, till we were able to work no longer, being already spent with labour while we were in the ship. We therefore trusted ourselves to the mercy of the waves, and in about half an hour the boat was overset by a sudden flurry from the north. What became of my companions in the boat, as well as of those who escaped on the rock, or were left in the vessel, I cannot tell; but conclude they were all lost. For my own part, I swam as fortune directed me, and was pushed forward by wind and tide. I often let my legs drop, and could feel no bottom; but when I was almost gone, and able to struggle no longer, I found myself within my depth; and by this time the storm was much abated. The declivity was so small, that I walked near a mile before I got to the shore, which I conjectured was about eight o'clock in the evening. I then advanced forward near half a mile, but could not discover any sign of houses or inhabitants; at least I was in so weak a condition, that I did not observe them. I was extremely tired, and with that, and the heat of the weather, and about half a pint of brandy that I drank as I left the ship, I found myself much inclined to sleep. I lay down on the grass, which was very short and soft, where I slept sounder than ever I remembered to have done in my life, and, as I reckoned, about nine hours; for when I awaked, it was just day-light. I attempted to rise, but was not able to stir: for, as I happened to lie on my back, I found my arms and legs were strongly fastened on each side to the ground; and my hair, which was long and thick, tied down in the same manner. I likewise felt several slender ligatures across my body, from my arm-pits to my thighs. I could only look upwards; the sun began to grow hot, and the light offended my eyes. I heard a confused noise about me; but in the posture I lay, could see nothing except the sky. In a little time I felt something alive moving on my left leg, which advancing gently forward over my breast, came almost up to my chin; when, bending my eyes downwards as much as I could, I perceived it to be a human creature not six inches high, with a bow and arrow in his hands, and a quiver at his back. In the mean time, I felt at least forty more of the same kind (as I conjectured) following the first. I was in the utmost astonishment, and roared so loud, that they all ran back in a fright; and some of them, as I was afterwards told, were hurt with the falls they got by leaping from my sides upon the ground. However, they soon returned, and one of them, who ventured so far as to get a full sight of my face, lifting up his hands and eyes by way of admiration, cried out in a shrill but distinct voice, _Hekinah degul_: the others repeated the same words several times, but then I knew not what they meant. I lay all this while, as the reader may believe, in great uneasiness.", 
	"At length, struggling to get loose, I had the fortune to break the strings, and wrench out the pegs that fastened my left arm to the ground; for, by lifting it up to my face, I discovered the methods they had taken to bind me, and at the same time with a violent pull, which gave me excessive pain, I a little loosened the strings that tied down my hair on the left side, so that I was just able to turn my head about two inches. But the creatures ran off a second time, before I could seize them; whereupon there was a great shout in a very shrill accent, and after it ceased I heard one of them cry aloud _Tolgo phonac_; when in an instant I felt above a hundred arrows discharged on my left hand, which, pricked me like so many needles; and besides, they shot another flight into the air, as we do bombs in Europe, whereof many, I suppose, fell on my body, (though I felt them not), and some on my face, which I immediately covered with my left hand. When this shower of arrows was over, I fell a groaning with grief and pain; and then striving again to get loose, they discharged another volley larger than the first, and some of them attempted with spears to stick me in the sides; but by good luck I had on a buff jerkin, which they could not pierce. I thought it the most prudent method to lie still, and my design was to continue so till night, when, my left hand being already loose, I could easily free myself: and as for the inhabitants, I had reason to believe I might be a match for the greatest army they could bring against me, if they were all of the same size with him that I saw. But fortune disposed otherwise of me. When the people observed I was quiet, they discharged no more arrows; but, by the noise I heard, I knew their numbers increased; and about four yards from me, over against my right ear, I heard a knocking for above an hour, like that of people at work; when turning my head that way, as well as the pegs and strings would permit me, I saw a stage erected about a foot and a half from the ground, capable of holding four of the inhabitants, with two or three ladders to mount it: from whence one of them, who seemed to be a person of quality, made me a long speech, whereof I understood not one syllable. But I should have mentioned, that before the principal person began his oration, he cried out three times, _Langro dehul san_ (these words and the former were afterwards repeated and explained to me); whereupon, immediately, about fifty of the inhabitants came and cut the strings that fastened the left side of my head, which gave me the liberty of turning it to the right, and of observing the person and gesture of him that was to speak. He appeared to be of a middle age, and taller than any of the other three who attended him, whereof one was a page that held up his train, and seemed to be somewhat longer than my middle finger; the other two stood one on each side to support him. He acted every part of an orator, and I could observe many periods of threatenings, and others of promises, pity, and kindness. I answered in a few words, but in the most submissive manner, lifting up my left hand, and both my eyes to the sun, as calling him for a witness; and being almost famished with hunger, having not eaten a morsel for some hours before I left the ship, I found the demands of nature so strong upon me, that I could not forbear showing my impatience (perhaps against the strict rules of decency) by putting my finger frequently to my mouth, to signify that I wanted food. The _hurgo_ (for so they call a great lord, as I afterwards learnt) understood me very well. He descended from the stage, and commanded that several ladders should be applied to my sides, on which above a hundred of the inhabitants mounted and walked towards my mouth, laden with baskets full of meat, which had been provided and sent thither by the king's orders, upon the first intelligence he received of me. I observed there was the flesh of several animals, but could not distinguish them by the taste. There were shoulders, legs, and loins, shaped like those of mutton, and very well dressed, but smaller than the wings of a lark. I ate them by two or three at a mouthful, and took three loaves at a time, about the bigness of musket bullets. They supplied me as fast as they could, showing a thousand marks of wonder and astonishment at my bulk and appetite. I then made another sign, that I wanted drink.",
	"They found by my eating that a small quantity would not suffice me; and being a most ingenious people, they slung up, with great dexterity, one of their largest hogsheads, then rolled it towards my hand, and beat out the top; I drank it off at a draught, which I might well do, for it did not hold half a pint, and tasted like a small wine of Burgundy, but much more delicious. They brought me a second hogshead, which I drank in the same manner, and made signs for more; but they had none to give me. When I had performed these wonders, they shouted for joy, and danced upon my breast, repeating several times as they did at first, _Hekinah degul_. They made me a sign that I should throw down the two hogsheads, but first warning the people below to stand out of the way, crying aloud, _Borach mevolah_; and when they saw the vessels in the air, there was a universal shout of _Hekinah degul_. I confess I was often tempted, while they were passing backwards and forwards on my body, to seize forty or fifty of the first that came in my reach, and dash them against the ground. But the remembrance of what I had felt, which probably might not be the worst they could do, and the promise of honour I made them - for so I interpreted my submissive behaviour - soon drove out these imaginations. Besides, I now considered myself as bound by the laws of hospitality, to a people who had treated me with so much expense and magnificence. However, in my thoughts I could not sufficiently wonder at the intrepidity of these diminutive mortals, who durst venture to mount and walk upon my body, while one of my hands was at liberty, without trembling at the very sight of so prodigious a creature as I must appear to them. After some time, when they observed that I made no more demands for meat, there appeared before me a person of high rank from his imperial majesty. His excellency, having mounted on the small of my right leg, advanced forwards up to my face, with about a dozen of his retinue; and producing his credentials under the signet royal, which he applied close to my eyes, spoke about ten minutes without any signs of anger, but with a kind of determinate resolution, often pointing forwards, which, as I afterwards found, was towards the capital city, about half a mile distant; whither it was agreed by his majesty in council that I must be conveyed. I answered in few words, but to no purpose, and made a sign with my hand that was loose, putting it to the other (but over his excellency's head for fear of hurting him or his train) and then to my own head and body, to signify that I desired my liberty. It appeared that he understood me well enough, for he shook his head by way of disapprobation, and held his hand in a posture to show that I must be carried as a prisoner. However, he made other signs to let me understand that I should have meat and drink enough, and very good treatment. Whereupon I once more thought of attempting to break my bonds; but again, when I felt the smart of their arrows upon my face and hands, which were all in blisters, and many of the darts still sticking in them, and observing likewise that the number of my enemies increased, I gave tokens to let them know that they might do with me what they pleased. Upon this, the _hurgo_ and his train withdrew, with much civility and cheerful countenances. Soon after I heard a general shout, with frequent repetitions of the words _Peplom selan_; and I felt great numbers of people on my left side relaxing the cords to such a degree, that I was able to turn upon my right, and to ease myself with making water; which I very plentifully did, to the great astonishment of the people; who, conjecturing by my motion what I was going to do, immediately opened to the right and left on that side, to avoid the torrent, which fell with such noise and violence from me. But before this, they had daubed my face and both my hands with a sort of ointment, very pleasant to the smell, which, in a few minutes, removed all the smart of their arrows. These circumstances, added to the refreshment I had received by their victuals and drink, which were very nourishing, disposed me to sleep. I slept about eight hours, as I was afterwards assured; and it was no wonder, for the physicians, by the emperor's order, had mingled a sleepy potion in the hogsheads of wine.",
	"It seems, that upon the first moment I was discovered sleeping on the ground, after my landing, the emperor had early notice of it by an express; and determined in council, that I should be tied in the manner I have related, (which was done in the night while I slept;) that plenty of meat and drink should be sent to me, and a machine prepared to carry me to the capital city. This resolution perhaps may appear very bold and dangerous, and I am confident would not be imitated by any prince in Europe on the like occasion. However, in my opinion, it was extremely prudent, as well as generous: for, supposing these people had endeavoured to kill me with their spears and arrows, while I was asleep, I should certainly have awaked with the first sense of smart, which might so far have roused my rage and strength, as to have enabled me to break the strings wherewith I was tied; after which, as they were not able to make resistance, so they could expect no mercy. These people are most excellent mathematicians, and arrived to a great perfection in mechanics, by the countenance and encouragement of the emperor, who is a renowned patron of learning. This prince has several machines fixed on wheels, for the carriage of trees and other great weights. He often builds his largest men of war, whereof some are nine feet long, in the woods where the timber grows, and has them carried on these engines three or four hundred yards to the sea. Five hundred carpenters and engineers were immediately set at work to prepare the greatest engine they had. It was a frame of wood raised three inches from the ground, about seven feet long, and four wide, moving upon twenty-two wheels. The shout I heard was upon the arrival of this engine, which, it seems, set out in four hours after my landing. It was brought parallel to me, as I lay. But the principal difficulty was to raise and place me in this vehicle. Eighty poles, each of one foot high, were erected for this purpose, and very strong cords, of the bigness of packthread, were fastened by hooks to many bandages, which the workmen had girt round my neck, my hands, my body, and my legs. Nine hundred of the strongest men were employed to draw up these cords, by many pulleys fastened on the poles; and thus, in less than three hours, I was raised and slung into the engine, and there tied fast. All this I was told; for, while the operation was performing, I lay in a profound sleep, by the force of that soporiferous medicine infused into my liquor. Fifteen hundred of the emperor's largest horses, each about four inches and a half high, were employed to draw me towards the metropolis, which, as I said, was half a mile distant.",
	"About four hours after we began our journey, I awaked by a very ridiculous accident; for the carriage being stopped a while, to adjust something that was out of order, two or three of the young natives had the curiosity to see how I looked when I was asleep; they climbed up into the engine, and advancing very softly to my face, one of them, an officer in the guards, put the sharp end of his half-pike a good way up into my left nostril, which tickled my nose like a straw, and made me sneeze violently; whereupon they stole off unperceived, and it was three weeks before I knew the cause of my waking so suddenly. We made a long march the remaining part of the day, and, rested at night with five hundred guards on each side of me, half with torches, and half with bows and arrows, ready to shoot me if I should offer to stir. The next morning at sun-rise we continued our march, and arrived within two hundred yards of the city gates about noon. The emperor, and all his court, came out to meet us; but his great officers would by no means suffer his majesty to endanger his person by mounting on my body. At the place where the carriage stopped there stood an ancient temple, esteemed to be the largest in the whole kingdom; which, having been polluted some years before by an unnatural murder, was, according to the zeal of those people, looked upon as profane, and therefore had been applied to common use, and all the ornaments and furniture carried away. In this edifice it was determined I should lodge. The great gate fronting to the north was about four feet high, and almost two feet wide, through which I could easily creep. On each side of the gate was a small window, not above six inches from the ground: into that on the left side, the king's smith conveyed fourscore and eleven chains, like those that hang to a lady's watch in Europe, and almost as large, which were locked to my left leg with six-and-thirty padlocks. Over against this temple, on the other side of the great highway, at twenty feet distance, there was a turret at least five feet high. Here the emperor ascended, with many principal lords of his court, to have an opportunity of viewing me, as I was told, for I could not see them. It was reckoned that above a hundred thousand inhabitants came out of the town upon the same errand; and, in spite of my guards, I believe there could not be fewer than ten thousand at several times, who mounted my body by the help of ladders. But a proclamation was soon issued, to forbid it upon pain of death. When the workmen found it was impossible for me to break loose, they cut all the strings that bound me; whereupon I rose up, with as melancholy a disposition as ever I had in my life. But the noise and astonishment of the people, at seeing me rise and walk, are not to be expressed. The chains that held my left leg were about two yards long, and gave me not only the liberty of walking backwards and forwards in a semicircle, but, being fixed within four inches of the gate, allowed me to creep in, and lie at my full length in the temple."
};

//! \class MeshPagesApp
//!
//!
class MeshPagesApp : public App {
public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	gl::SdfText::Font						mFont;
	gl::SdfTextRef							mSdfText;
	gl::SdfTextMeshRef						mSdfTextMesh;
	bool									mPremultiply = true;
	gl::SdfText::Alignment					mAlignment = gl::SdfText::Alignment::LEFT;
	bool									mJustify = true;
	bool									mDrawBounds = false;

	std::vector<gl::SdfTextMesh::RunRef>	mPageNumRuns;
	std::vector<gl::SdfTextMesh::RunRef>	mPageTextRuns;
	
	Anim<vec2>								mOffset = vec2( 0, 0 );

	void moveToPage( uint32_t pageNum );
};

void MeshPagesApp::setup()
{
#if defined( CINDER_MSW )
	// For AllSamples
	addAssetDirectory( getAppPath() / "../../../../MeshPages/assets" );
#endif

	mFont = gl::SdfText::Font( loadAsset( "Alike-Regular.ttf" ), 18 );
	mSdfText = gl::SdfText::create( getAssetPath( "" ) / "Alike.sdft", mFont );
	mSdfTextMesh = gl::SdfTextMesh::create();

	auto pageNumOptions = gl::SdfTextMesh::Run::Options().setDrawScale( 0.75f );	
	auto pageTextOptions = gl::SdfTextMesh::Run::Options().setAlignment( mAlignment ).setJustify( mJustify ).setLeading( -2.0f );
	auto pageTextFitRect = Rectf( 0, 0, kPageTextWidth, kPageTextWidth ) + vec2( kPageBorder, 6.5f * kPageBorder );

	for( int i = 0; i < sStrings.size(); ++i ) {
		std::string pageStr = "Page " + toString( i + 1 );
		vec2 baseline = vec2( i * kPageWidth + kPageBorder, 4.5f * kPageBorder );
		auto pageNumRun = mSdfTextMesh->appendText( pageStr, mSdfText, baseline, pageNumOptions );
		mPageNumRuns.push_back( pageNumRun );

		const auto& pageTextStr = sStrings[i];
		auto fitRect = pageTextFitRect + vec2( i * kPageWidth, 0 );
		auto pageTextRun = mSdfTextMesh->appendTextWrapped( pageTextStr, mSdfText, fitRect, pageTextOptions );
		mPageTextRuns.push_back( pageTextRun );
	}
}

void MeshPagesApp::moveToPage( uint32_t pageNum )
{
	vec2 value = vec2( -( ( pageNum - 1 ) * kPageWidth ), 0 );
	timeline().apply( &mOffset, value, 0.5f, EaseOutExpo() );
}

void MeshPagesApp::keyDown( KeyEvent event )
{
	switch( event.getChar() ) {
		case '=':
		case '+':
			mFont = gl::SdfText::Font( mFont.getName(), mFont.getSize() + 1 );
			mSdfText = gl::SdfText::create( mFont );
		break;
		case '-':
			mFont = gl::SdfText::Font( mFont.getName(), mFont.getSize() - 1 );
			mSdfText = gl::SdfText::create( mFont );
		break;
		case 'p':
		case 'P':
			mPremultiply = ! mPremultiply;
		break;
		case 'l':
		case 'L':
			mAlignment = gl::SdfText::Alignment::LEFT;
			for( auto& run : mPageTextRuns ) {
				run->setAlignment( mAlignment );
			}
		break;
		case 'r':
		case 'R':
			mAlignment = gl::SdfText::Alignment::RIGHT;
			for( auto& run : mPageTextRuns ) {
				run->setAlignment( mAlignment );
			}
		break;
		case 'c':
		case 'C':
			mAlignment = gl::SdfText::Alignment::CENTER;
			for( auto& run : mPageTextRuns ) {
				run->setAlignment( mAlignment );
			}
		break;
		case 'j':
		case 'J':
			mJustify = ! mJustify;
			for( auto& run : mPageTextRuns ) {
				run->setJustify( mJustify );
			}
		break;

		case 'b':
		case 'B':
			mDrawBounds = ! mDrawBounds;
		break;

		case '1': moveToPage( 1 );  break;
		case '2': moveToPage( 2 );  break;
		case '3': moveToPage( 3 );  break;
		case '4': moveToPage( 4 );  break;
		case '5': moveToPage( 5 );  break;
		case '6': moveToPage( 6 );  break;
		case '7': moveToPage( 7 );  break;
		case '8': moveToPage( 8 );  break;
		case '9': moveToPage( 9 );  break;
		case '0': moveToPage( 10 ); break;
	}
}

void MeshPagesApp::mouseDown( MouseEvent event )
{
}

void MeshPagesApp::update()
{
}

void MeshPagesApp::draw()
{
	gl::setMatricesWindow( getWindowSize() );
	gl::enableAlphaBlending();
	gl::clear( Color( 0.93f, 0.92f, 0.9f ) );

	auto drawOptions = gl::SdfText::DrawOptions().premultiply( mPremultiply );

	if( mDrawBounds ) {
		gl::ScopedModelMatrix scopedModel;
		gl::translate( mOffset.value() );

		auto runs = mSdfTextMesh->getRuns();		
		gl::lineWidth( 1.0f );
		gl::color( Color( 0.1f, 0.8f, 0.9f ) );
		for( const auto &run : runs ) {
			const auto &bounds = run->getBounds();
			gl::drawStrokedRect( bounds );
		}
	}

	gl::color( Color( 0.1f, 0.1f, 0.1f ) );
	drawOptions.scale( 2.0f );
	mSdfText->drawString( "Gulliver's Travels into Several Remote Nations of the World", vec2( 1.5f * kPageBorder, 2.5f * kPageBorder ), drawOptions );

	{
		gl::ScopedModelMatrix scopedModel;
		gl::translate( mOffset.value() );
		mSdfTextMesh->draw( mPremultiply );
	}

	// Draw FPS
	gl::color( Color( 0.4f, 0.4f, 0.4f ) );
	drawOptions.scale( 1.25f );
	mSdfText->drawString( toString( floor( getAverageFps() ) ) + " FPS | " + std::string( mPremultiply ? "premult" : "" ), vec2( 10, getWindowHeight() - mSdfText->getDescent() ), drawOptions );

}

void prepareSettings( App::Settings *settings )
{
	settings->setWindowSize( 1024, 1024 + kPageBorder );
}

CINDER_APP( MeshPagesApp, RendererGl, prepareSettings );
